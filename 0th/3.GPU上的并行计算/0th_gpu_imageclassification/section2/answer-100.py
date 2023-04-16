import torch
import torch.nn as nn
import torch.optim as optim
from torch.autograd import Variable
from torch.utils.data import DataLoader
from torch.utils.data import sampler
from torch.utils.data import random_split

import torchvision.datasets as dset
import torchvision.transforms as T
import torchvision.models as models
import torchvision.io as io

import numpy as np
import pickle
import timeit
import copy
import os
import PIL.Image

class ChunkSampler(sampler.Sampler):
    """Samples elements sequentially from some offset. 
    Arguments:
        num_samples: # of desired datapoints
        start: offset where we should start selecting from
    """
    def __init__(self, num_samples, start = 0):
        self.num_samples = num_samples
        self.start = start

    def __iter__(self):
        return iter(range(self.start, self.start + self.num_samples))

    def __len__(self):
        return self.num_samples

trans = T.Compose([
    T.Resize((64,64)),
    T.ToTensor(),
    T.Normalize(mean=[0.4914, 0.4822, 0.4465],std=[0.2023, 0.1994, 0.2010])
])

datas = dset.ImageFolder("./data/plant-seedling/data", transform=trans)

print(datas.classes)
print(datas.class_to_idx)
train, val = random_split(datas, [0.9,0.1], generator=torch.Generator().manual_seed(20230118))

loader_train = DataLoader(train, batch_size=64)
loader_val = DataLoader(val, batch_size=64)

dtype = torch.cuda.FloatTensor # the GPU datatype

# Constant to control how frequently we print train loss
print_every = 10

simple_model = models.resnet34(models.ResNet34_Weights.DEFAULT)
simple_model.fc = nn.Linear(512, 12, bias=True)
simple_model = simple_model.type(dtype)
print(simple_model)

loss_fn = nn.CrossEntropyLoss().type(dtype)
optimizer = optim.SGD(simple_model.parameters(), lr=5e-3,
                      momentum=0.9, weight_decay=5e-4)
scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=200)

def check_accuracy(model, loader):
    num_correct = 0
    num_samples = 0
    model.eval() # Put the model in test mode (the opposite of model.train(), essentially)
    for x, y in loader:
        x_var = Variable(x.type(dtype), volatile=True)
        scores = model(x_var)
        _, preds = scores.data.cpu().max(1)
        num_correct += (preds == y).sum()
        num_samples += preds.size(0)
    acc = float(num_correct) / num_samples
    print('Got %d / %d correct (%.2f)' % (num_correct, num_samples, 100 * acc))

def train(model, loss_fn, optimizer, num_epochs = 1):
    for epoch in range(num_epochs):
        print('Starting epoch %d / %d' % (epoch + 1, num_epochs))
        model.train()
        for t, (x, y) in enumerate(loader_train):
            x_var = Variable(x.type(dtype))
            y_var = Variable(y.type(dtype).long())
            
            scores = model(x_var)
            
            loss = loss_fn(scores, y_var)
            if (t + 1) % print_every == 0:
                print('t = %d, loss = %.4f' % (t + 1, loss))

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
        check_accuracy(model, loader_val)
        torch.save(model.state_dict(), "section2-{}.ckpt".format(epoch))
        scheduler.step()

def cal_result(model):
    model.eval()
    TEST_DIR = "/data/shared/plant-seedling"
    csv_buf = "file,species\n"
    for file in os.listdir(TEST_DIR):
        img = PIL.Image.open(TEST_DIR + "/" + file).convert('RGB')
        img = trans(img)
        x_var = Variable(img.type(dtype), volatile=True)
        scores = model(x_var.unsqueeze(0))
        _, preds = scores.data.cpu().max(1)
        csv_buf += "{},{}\n".format(file, datas.classes[preds[0]])
    with open("submission.csv","w") as file:
        file.write(csv_buf)

train(simple_model, loss_fn, optimizer, 30)
#simple_model.load_state_dict(torch.load("section2-29.ckpt"))
#check_accuracy(simple_model, loader_val)
cal_result(simple_model)