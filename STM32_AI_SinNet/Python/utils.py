import torch
from torch import nn
from torch.utils.data import DataLoader, TensorDataset
from torch.optim.lr_scheduler import ExponentialLR

import numpy as np
from sklearn.model_selection import train_test_split

import os
import time
import datetime
import argparse

import onnxruntime
import matplotlib.pyplot as plt

from SinNet import SinNet


def train(train_outputs: dict, args: argparse.Namespace) -> SinNet:    
    device = torch.device("cuda") if torch.cuda.is_available() else torch.device("cpu")
    print(f"Training on: {device}")
    
    X = torch.linspace(0, torch.pi * 2, args.num_samples) # creates a sequence of num_samples steps between 0 and 2pi
    Y = np.sin(X)
    print(f"Training SinNet with {args.num_samples} samples")
    
    train_outputs["mean"] = 0
    train_outputs["std"] = 1
    if args.normalize_input:
        print("Normalization has been activated")
        train_outputs["mean"] = X.mean().item()
        train_outputs["std"] = X.std().item()
        
    X = (X - train_outputs["mean"]) / train_outputs["std"] 
    
    # dataset creation with random samples in [0, 2pi], while labels are their corresponding sin value between [-1,1]
    X_train, X_test, Y_train, Y_test = map(torch.tensor, train_test_split(X, Y, test_size=0.2))
    train_loader = DataLoader(TensorDataset(X_train.unsqueeze(1), Y_train.unsqueeze(1)), 
                             batch_size=args.batch_size, 
                             pin_memory=True, 
                             shuffle=True,
                             num_workers=args.num_workers,
                             persistent_workers=args.num_workers > 0) 
    test_loader = DataLoader(TensorDataset(X_test.unsqueeze(1), Y_test.unsqueeze(1)), 
                            batch_size=args.batch_size, 
                            pin_memory=True, 
                            shuffle=False,
                            num_workers=args.num_workers,
                            persistent_workers=args.num_workers > 0) 
    
    print("\nSetting up the network, criterion and optimizer with the following parameters:")
    print(f"\t-learning rate: {args.learning_rate}")
    print(f"\t-dropout probability: {args.p_dropout}")
    print(f"\t-hidden layer size: {args.hidden_size}")

    net = SinNet(args.hidden_size, args.p_dropout).to(device)
    criterion = nn.SmoothL1Loss(reduction="mean")
    optimizer = torch.optim.Adam(net.parameters(), lr=args.learning_rate)
    scheduler = ExponentialLR(optimizer, gamma=0.9)
    
    train_losses = list()
    test_losses = list()
    
    print(f"\nStarting training on {args.num_epochs} epochs:")
    for e in range(args.num_epochs):
        
        # TRAINING STEP
        net.train()
        current_losses = list()
        for x, y in train_loader:
            x = x.to(torch.float32).to(device)
            y = y.to(torch.float32).to(device)
            
            optimizer.zero_grad()
            
            predictions = net(x)
            loss = criterion(predictions, y)
            loss.backward()
            optimizer.step()
            
            current_losses.append(loss.detach().cpu().item())
        
        scheduler.step()    
        train_losses.append(sum(current_losses)/len(current_losses))
        
        # VALIDATION STEP
        net.eval()
        with torch.no_grad():
            for x, y in test_loader:
                x = x.to(torch.float32).to(device)
                y = y.to(torch.float32).to(device)
                
                predictions = net(x)
                loss = criterion(predictions, y)                
                current_losses.append(loss.detach().cpu().item())
                
            test_losses.append(sum(current_losses)/len(current_losses))
        
        print(f"EPOCH: {e}|{args.num_epochs} - Average train loss: {train_losses[e]:.5f} - Average test loss: {test_losses[e]:.5f}")
    print("training done!")
    
    now = datetime.datetime.now()
    run_path = os.path.join(train_outputs["base_path"], now.strftime("run_%d_%m_%Y__%H_%M_%S"))
    os.makedirs(run_path)
    print(f"\nExported model to {run_path}")
    
    torch.save(net.state_dict(), os.path.join(run_path, "model_fp32.pth"))
    
    train_outputs["model"] = net
    train_outputs["run_path"] = run_path
    return train_outputs


def eval_model(model: SinNet, device: str, train_outputs: dict, args: argparse.Namespace) -> None:
    assert device in ["cuda", "cpu"], "the specified device is invalid"
    device = torch.device("cuda") if torch.cuda.is_available() and device == "cuda" else torch.device("cpu")
   
    print(f"\nStarting evaluation on {10**4} samples")
    X = torch.linspace(0, 2 * torch.pi, 10**4)
    Y = torch.sin(X)
    X = (X - train_outputs["mean"]) / train_outputs["std"]
    
    eval_loader = DataLoader(TensorDataset(X.unsqueeze(1), Y.unsqueeze(1)), batch_size=1, pin_memory=True, shuffle=True) 
    criterion = nn.SmoothL1Loss(reduction="mean")
    
    model.eval()
    model.to(device)
    losses = list()
    elapsed_time = 0
    with torch.no_grad():
        for x, y in eval_loader:
            x = x.to(device)
            y = y.to(device)
            
            start = time.time()
            predictions = model(x)
            stop = time.time()
            elapsed_time += stop - start
            
            loss = criterion(predictions, y)                
            losses.append(loss.detach().cpu().item())
    elapsed_time *= 1000
            
    print(f"Model loss: {sum(losses)/len(losses)} - Run time AVG: {elapsed_time/len(eval_loader):.3f}ms")


def load_most_recent_model(outputs: dict, args: argparse.Namespace) -> SinNet:
    if os.path.exists(outputs["base_path"]):
        runs = [d for d in os.listdir(outputs["base_path"]) if os.path.isdir(os.path.join(outputs["base_path"], d))]
        runs.sort(key=lambda d: os.path.getmtime(os.path.join(outputs["base_path"], d)), reverse=True)

        if runs:
            recent_run_path = os.path.join(outputs["base_path"], runs[0])
            model_path = os.path.join(recent_run_path, 'model_fp32.pth')
            print(f"\Loaded model: {model_path}")
            
            if os.path.exists(model_path):
                sinNet = SinNet(args.hidden_size, args.p_dropout)
                sinNet.load_state_dict(torch.load(model_path))
                outputs["model"] = sinNet
                outputs["run_path"] = recent_run_path
                return outputs
            else:
                raise RuntimeError("The model file is not present in the most recent directory")
        else:
            raise RuntimeError("No 'runs' directory found")
    else:
        raise RuntimeError("The specified folder does not exists")
    
    
def to_numpy(tensor: torch.Tensor) -> np.ndarray:
    return tensor.detach().cpu().numpy() if tensor.requires_grad else tensor.cpu().numpy()
    
    
def test_onnx_model(model: SinNet, onnx_model_path: str, onnxruntime_provider: list, train_outputs: dict, args: argparse.Namespace) -> None:
    x = torch.linspace(0, torch.pi * 2, args.batch_size).unsqueeze(1).to("cpu")
    x = (x - train_outputs["mean"]) / train_outputs["std"]
    
    print(f"\nStarting evaluation of ONNX model on {args.batch_size} samples")
    model.eval()
    predictions = model(x).to("cpu")
    ort_session = onnxruntime.InferenceSession(onnx_model_path, providers=onnxruntime_provider)

    # compute ONNX Runtime output prediction
    ort_inputs = {ort_session.get_inputs()[0].name: to_numpy(x)}
    
    start = time.time()
    onnxruntime_predictions = ort_session.run(None, ort_inputs)
    end = time.time()
    elapsed_time = (end - start) * 1000
    
    print(f"Run time AVG ONNX model: {elapsed_time/args.batch_size:.3f}ms")

    # compare ONNX Runtime and PyTorch results
    try:
        np.testing.assert_allclose(to_numpy(predictions), onnxruntime_predictions[0], rtol=1e-02, atol=1e-04)
    except Exception as e:
        print(e)
        

def visualize_onnx_results(train_outputs: dict, onnx_model_path: str, title: str = "Predicted sin vs real sin function"):

    sess = onnxruntime.InferenceSession(onnx_model_path)

    x_fp32 = np.random.uniform(0, 2 * np.pi, 5000).astype(np.float32).reshape(-1, 1)
    y_true = np.sin(x_fp32)

    mean = train_outputs["mean"].cpu().numpy() if hasattr(train_outputs["mean"], "cpu") else train_outputs["mean"]
    std  = train_outputs["std"].cpu().numpy()  if hasattr(train_outputs["std"],  "cpu") else train_outputs["std"]
    x_norm = (x_fp32 - mean) / std

    y_pred = sess.run(None, {'input': x_norm.astype(np.float32)})[0]

    plt.figure(figsize=(6,4))
    plt.scatter(x_fp32, y_true, label='True sin(x)',      s=0.2)
    plt.scatter(x_fp32, y_pred, label='Predicted sin(x)', s=0.2)
    plt.xlabel('Input (radians)')
    plt.ylabel('Function value')
    plt.title(title)
    plt.legend()

    out_dir = os.path.join(train_outputs["run_path"], "images")
    os.makedirs(out_dir, exist_ok=True)
    plt.savefig(os.path.join(out_dir, f"{title}.png"))
    plt.close()