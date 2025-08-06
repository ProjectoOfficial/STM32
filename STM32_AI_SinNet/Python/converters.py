import torch
import onnx

import os
import argparse
import numpy as np

from utils import test_onnx_model

def convert_to_onnx(train_outputs: dict, args: argparse.Namespace) -> None:
    print("\nConverting model to ONNX format")
    
    model = train_outputs["model"] # pytorch float32 model
    model.eval()
    model.to("cpu")
    
    calibration_set = torch.randn(args.batch_size, 1, requires_grad=True).to("cpu") * 2 * np.pi
    
    onnx_model_path = os.path.join(train_outputs["run_path"], "model.onnx")
    torch.onnx.export(model,                    # model being run
                  calibration_set,              # model input (or a tuple for multiple inputs)
                  onnx_model_path,              # where to save the model (can be a file or file-like object)
                  export_params=True,           # store the trained parameter weights inside the model file
                  opset_version=17,             # the ONNX version to export the model to
                  do_constant_folding=True,     # whether to execute constant folding for optimization
                  input_names = ['input'],      # the model's input names
                  output_names = ['output'],    # the model's output names
                  dynamic_axes={'input' : {0 : 'batch_size'},    # variable length axes
                                'output' : {0 : 'batch_size'}},
                  )
    
    
    onnx_model = onnx.load(onnx_model_path)
    onnx.checker.check_model(onnx_model)
    train_outputs["onnx_model"] = onnx_model

    test_onnx_model(model, 
                    onnx_model_path, 
                    ["CPUExecutionProvider", 'CUDAExecutionProvider'], 
                    train_outputs, args)
    
    print("Exported model has been tested with ONNXRuntime!")
    print(f"ONNX model stored at {onnx_model_path}")
    
    if args.export_to_tflite:
        raise NotImplementedError("As always Tensorflow causes issues with no reasons due to dependency version mismatches")
        # tf_model_path = os.path.join(run_path, "model.tf")
        # tf_model = onnx_tf.backend.prepare(onnx_model)
        # tf_model.export_graph(tf_model_path)
        
        # tflite_model_path = os.path.join(run_path, "model.tflite")
        # converter = tf.lite.TFLiteConverter.from_saved_model(tf_model_path)
        # tflite_model = converter.convert()
        # open(tflite_model_path, 'wb').write(tflite_model)
        # print("Model exported to tflite!")
        
    print("Conversions done!")
    return train_outputs