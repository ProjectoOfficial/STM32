import argparse
import random
import multiprocessing

import torch
import numpy as np

import os
from quantization import quantize_model_onnx
from converters import convert_to_onnx
from utils import train, load_most_recent_model, visualize_onnx_results


def main(args: argparse.Namespace):
    if args.num_workers > 0:
        multiprocessing.set_start_method('fork', force=True)
        
    outputs = dict()
    outputs["run_path"] = None
    outputs["base_path"] = os.path.join(os.path.dirname(os.path.realpath(__file__)), "experiments")
    outputs["mean"] = 0
    outputs["std"] = 1
    
    if not os.path.exists(outputs["base_path"]):
        os.makedirs(outputs["base_path"])
        
    if args.train:
        outputs = train(outputs, args)
    else:
        outputs = load_most_recent_model(outputs, args)
        
    if args.export_to_onnx:
        outputs = convert_to_onnx(outputs, args) # ONNX does not support conversion on pytorch quantized models
    
    if args.quantize:
        outputs = quantize_model_onnx(outputs, args)
        
    if args.visualize_onnx_results:
        onnx_path = os.path.join(outputs["run_path"], "model.onnx")
        assert os.path.exists(onnx_path), "You need first to convert pytorch .pt file in onnx"
        visualize_onnx_results(outputs, onnx_path, "model fp32 onnx")
        
        
        onnx_path = os.path.join(outputs["run_path"], "model_int8.onnx")
        assert os.path.exists(onnx_path), "You need first to generate the quantized model"
        visualize_onnx_results(outputs, onnx_path, "model int8 onnx")
        
    
def set_seed(args: argparse.Namespace):
    random.seed(args.random_seed)
    torch.manual_seed(args.random_seed)
    np.random.seed(args.random_seed)
    
    if torch.cuda.is_available():
        torch.cuda.manual_seed(args.random_seed)
        torch.cuda.manual_seed_all(args.random_seed)
        
        torch.backends.cudnn.deterministic = True
        torch.backends.cudnn.benchmark = False
    
if __name__ == "__main__":    
    parser = argparse.ArgumentParser(description="SineNet Approximator")
    parser.add_argument("--train", action="store_true", help="use it if you want to train the network from scratch")
    parser.add_argument("--normalize-input", action="store_true", help="use it if you want to normalize the input data (not recommended)")
    parser.add_argument("--quantize", action="store_true", help="use it if you want to use pytorch's quantization")
    parser.add_argument("--export-to-onnx", action="store_true", help="use it if you want to convert the model to ONNX (it also creates a quantized version of the ONNX network)")
    parser.add_argument("--export-to-tflite", action="store_true", help="use it if you want to convert the model to tflite (currently not tested)")
    parser.add_argument("--visualize-onnx-results", action="store_true", help="use it if you want to visualize model's prediction (ONNX + ONNX quantized)")
    
    
    parser.add_argument("-bs", "--batch-size", type=int, default=512, help="size of the batch of samples provided as input to the network during training")
    parser.add_argument("-hs", "--hidden-size", type=int, default=1024, help="number of neurons within the hidden layer")
    parser.add_argument("-lr", "--learning-rate", type=float, default=1e-4, help="learning rate (or step size) used for gradient descent optimization in Adam")
    parser.add_argument("-ne", "--num-epochs", type=int, default=10, help="number of epochs which will be used to train the network")
    parser.add_argument("-ns", "--num-samples", type=int, default=10**6, help="number of steps used to create the dataset (values between 0 and 2pi)")
    parser.add_argument("-pd", "--p-dropout", type=float, default=0.1, help="regularization parameter which adds a dropout probability in the input and hidden layers")
    parser.add_argument("-nw", "--num-workers", type=int, default=4, help="number of worker processes for data loading")
    parser.add_argument("-s", "--random-seed", type=int, default=41, help="integer number representing the seed of the random distribution used by random functions")
    args = parser.parse_args()
    
    set_seed(args)
    main(args)