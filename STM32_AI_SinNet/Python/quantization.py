import argparse
import os
import torch
import numpy as np
import onnx
import onnxruntime
from onnxruntime.quantization import (
    quantize_static,
    QuantType,
    CalibrationMethod,
    CalibrationDataReader,
    shape_inference
)

from utils import test_onnx_model
from SinNet import SinNet

class QuantizationDataReader(CalibrationDataReader):
    def __init__(self, dataset: torch.Tensor, batch_size: int, input_name: str):
        self.dl = torch.utils.data.DataLoader(dataset, batch_size=batch_size, shuffle=False)
        self.input_name = input_name
        self.iter_dl = iter(self.dl)

    def to_numpy(self, t: torch.Tensor) -> np.ndarray:
        return t.cpu().numpy()

    def get_next(self):
        batch = next(self.iter_dl, None)
        if batch is None:
            return None
        return {self.input_name: self.to_numpy(batch)}

    def rewind(self):
        self.iter_dl = iter(self.dl)


def quantize_model_onnx(train_outputs: dict, args: argparse.Namespace) -> dict:
    print("Starting ONNX static quantization (INT8)")

    run = train_outputs["run_path"]
    fp32_path  = os.path.join(run, "model.onnx")
    prep_path  = os.path.join(run, "model_prep.onnx")
    int8_path  = os.path.join(run, "model_int8.onnx")

    shape_inference.quant_pre_process(fp32_path, prep_path)

    sess = onnxruntime.InferenceSession(fp32_path, providers=["CPUExecutionProvider"])
    input_name = sess.get_inputs()[0].name
    calib_set  = torch.rand(args.num_samples // 5, 1) * 2 * torch.pi
    qdr = QuantizationDataReader(calib_set, batch_size=1, input_name=input_name)

    quantize_static(
        model_input=prep_path,
        model_output=int8_path,
        calibration_data_reader=qdr,
        weight_type=QuantType.QInt8,
        activation_type=QuantType.QInt8,
        calibrate_method=CalibrationMethod.Entropy,
        extra_options={
            "ActivationSymmetric": True,
            "WeightSymmetric": True
        }
    )

    onnx.checker.check_model(int8_path)

    torch_model = SinNet(args.hidden_size, args.p_dropout)
    torch_model.load_state_dict(torch.load(os.path.join(run, "model_fp32.pth"), map_location="cpu"))

    test_onnx_model(
        torch_model,
        onnx_model_path=int8_path,
        onnxruntime_provider=["CPUExecutionProvider", "CUDAExecutionProvider"],
        train_outputs=train_outputs,
        args=args
    )

    train_outputs["onnx_model_int8_path"] = int8_path
    print(f"Quantization done, saved at: {int8_path}")
    return train_outputs
