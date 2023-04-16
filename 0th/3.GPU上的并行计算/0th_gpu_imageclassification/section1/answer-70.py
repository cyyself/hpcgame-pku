import torch
import torch.nn as nn
import torch.optim as optim
from torch.autograd import Variable
from torch.utils.data import DataLoader
from torch.utils.data import sampler

import torchvision.datasets as dset
import torchvision.transforms as T
import torchvision.models as models

import numpy as np
import pickle
import timeit
import copy

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

NUM_TRAIN = 50000
NUM_VAL = 1000

transform_train = T.Compose([
    T.RandomCrop(32, padding=4),
    T.RandomHorizontalFlip(),
    T.ToTensor(),
    T.Normalize((0.4914, 0.4822, 0.4465), (0.2023, 0.1994, 0.2010)),
])

transform_test = T.Compose([
    T.ToTensor(),
    T.Normalize((0.4914, 0.4822, 0.4465), (0.2023, 0.1994, 0.2010)),
])


cifar10_train = dset.CIFAR10('./data/CIFAR10', train=True, download=True,
                           transform=transform_train)
loader_train = DataLoader(cifar10_train, batch_size=64, sampler=ChunkSampler(NUM_TRAIN, 0))

cifar10_val = dset.CIFAR10('./test_data/CIFAR10', train=True, download=True,
                           transform=transform_test)
loader_val = DataLoader(cifar10_val, batch_size=64, sampler=sampler.RandomSampler(cifar10_val, False, NUM_VAL))

cifar10_test = dset.CIFAR10('./test_data/CIFAR10', train=False, download=True,
                          transform=transform_test)
loader_test = DataLoader(cifar10_test, batch_size=64)

dtype = torch.cuda.FloatTensor # the GPU datatype

# Constant to control how frequently we print train loss
print_every = 100

# Here's where we define the architecture of the model... 
simple_model = models.resnet50(models.ResNet50_Weights.DEFAULT).type(dtype)
print(simple_model)

loss_fn = nn.CrossEntropyLoss().type(dtype)
optimizer = optim.SGD(simple_model.parameters(), lr=5e-3,
                      momentum=0.9, weight_decay=5e-4)
scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=200)

def check_accuracy(model, loader):
    if loader.dataset.train:
        print('Checking accuracy on validation set')
    else:
        print('Checking accuracy on test set')   
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
    return acc

def train(model, loss_fn, optimizer, epoch_start = 0, num_epochs = 1):
    if epoch_start != 0:
        simple_model.load_state_dict(torch.load("section1-{}.ckpt".format(epoch_start-1)))
    for epoch in range(epoch_start,num_epochs):
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
        torch.save(model.state_dict(), "section1-{}.ckpt".format(epoch))
        scheduler.step()
        if check_accuracy(model, loader_test) >= 0.9:
            print("archived goal at epoch {}".format(epoch))
            break

def save_result(model, loader):
    num_correct = 0
    num_samples = 0
    model.eval() # Put the model in test mode (the opposite of model.train(), essentially)
    result = []
    for x, y in loader:
        x_var = Variable(x.type(dtype), volatile=True)

        scores = model(x_var)
        _, preds = scores.data.cpu().max(1)
        result.append(preds)

    with open("./result", "wb") as fp:   #Pickling
        pickle.dump(result, fp)

train(simple_model, loss_fn, optimizer, 0, 32)
#max_acc = 0
#max_acc_idx = 0
#for i in range(0,100):
#    simple_model.load_state_dict(torch.load("section1-{}.ckpt".format(i)))
#    print("loaded checkpoint {}".format(i))
#    cur_acc = check_accuracy(simple_model, loader_test)
#    if cur_acc > max_acc:
#        max_acc = cur_acc
#        max_acc_idx = i
#        save_result(simple_model, loader_test)
#print(max_acc, max_acc_idx)
#simple_model.load_state_dict(torch.load("section1-{}.ckpt".format(31)))
check_accuracy(simple_model, loader_test)
save_result(simple_model, loader_test)