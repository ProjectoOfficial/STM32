import torch
from torch import nn


class SinNet(nn.Module):
    def __init__(self, hidden_size: int= 1024, p_dropout=0.2) -> None:
        super(SinNet, self).__init__()
        self.hidden_size = hidden_size
        self.p_dropout = p_dropout
        
        self.network = nn.Sequential(
            nn.Linear(1, self.hidden_size),
            nn.BatchNorm1d(self.hidden_size),
            nn.ReLU(),
            nn.Dropout(self.p_dropout),

            nn.Linear(self.hidden_size, self.hidden_size),
            nn.BatchNorm1d(self.hidden_size),
            nn.ReLU(),
            nn.Dropout(self.p_dropout),

            nn.Linear(self.hidden_size, self.hidden_size),
            nn.BatchNorm1d(self.hidden_size),
            nn.ReLU(),
            nn.Dropout(self.p_dropout),
            
            nn.Linear(self.hidden_size, 1),
            nn.BatchNorm1d(1),
        )
        
    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = self.network(x)
        return x