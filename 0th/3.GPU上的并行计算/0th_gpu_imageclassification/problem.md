*It is recommended to read 'How to submit' session before you begin working on it.*

# Image Classification

# **Section 1** Can you train a simple neural network using Pytorch?

In this section, you need to complete the code of **Pytorch.ipynb**. You can see more tutorials and details in the file. We will check the completion and correctness of your code in this section.


# **Section 2** Can you differentiate a weed from a crop seedling?

With the network training experience, we can solve a lot of interesting problems and let's try one of them in this section.

The ability to do so effectively can mean better crop yields and better stewardship of the environment.

The Aarhus University Signal Processing group, in collaboration with University of Southern Denmark, has recently released a dataset containing images of approximately 960 unique plants belonging to 12 species at several growth stages.

<p align="center">
  <img src='https://hpcgame.pku.edu.cn/oss/images/ai0/seeds.png' width="800"/><br>
  Fig1: Some examples of seedlings.
</p>



We're hosting this dataset as a problem in order to give it wider exposure, to give the community an opportunity to experiment with different image recognition techniques, as well to provide a place to cross-pollenate ideas.



## Dataset Description

You are provided with a training set and a test set of images of plant seedlings at various stages of grown. Each image has a filename that is its unique id. The dataset comprises 12 plant species. The goal of the competition is to create a classifier capable of determining a plant's species from a photo. The list of species is as follows:

```
Black-grass
Charlock
Cleavers
Common Chickweed
Common wheat
Fat Hen
Loose Silky-bent
Maize
Scentless Mayweed
Shepherds Purse
Small-flowered Cranesbill
Sugar beet
```

You **need to** organize the dataset in a **proper** way for your network training. In your network training, you need to use some training data to train your model and use some validation data to evaluate the performance of your model and choose the best checkpoint with the best classification accuracy. For more information of the dataset, you can load the images and check it by yourself.

## Network Training

You need to write your own code and finish the network training. You can use your previous knowledge or what you learned from the first section to improve the performance of your model. And some potential tricks include:

- better model architecture
- data augmentation
- batch norm
- dropout
- different activation functions


The final grade will be determined by the test results on an unseen test dataset. However, rest assured that the test results are consistent with the same distribution as our dataset and will be roughly similar to the performance of our validation dataset. We will also eventually check and **deduct points** if the code implementation is more problematic, e.g., if the checkpoint obtained is not possible to obtain from the implemented code

You need to submit your prediction result in the same format as the example submission file, **sample_submission.csv**. We will give your model's accuracy and a corresponding score for this problem.

# Useful materials

- [pytorch tutorial](https://pytorch.org/tutorials/)
- pytorch cheat sheet (see in attachments)

# Acknowledgments

We extend our appreciation to the Aarhus University Department of Engineering Signal Processing Group for hosting the original data and we also refer to some tutorials from Stanford University.


# Citation

A Public Image Database for Benchmark of Plant Seedling Classification Algorithms

Kaggle Competition

cs231n course, Stanford University

========== 编者注 ==========
## 任务说明
Section1 给与大家充分的学习和热身，在简单数据集合上实现ai训练，并给与了充分的代码和指导。Section2 旨在考验利用Section1的训练经验自己实现整个ai训练的任务，所以需要大家自己组织已给数据集，将其划分为自己认为合适的集合和格式，自己设计模型书写代码完成训练。

## 如何提交

提交一个压缩包，其中包括两个文件和一个文件夹。

### 文件：

- `result`：第一部分的训练结果，通过`save_result(model, loader_test)`生成
- `submission.csv`：识别`/data/shared/plant-seedling/`中文件的结果，保存为csv格式，格式可参考`sample_submission.csv`，其中第一行将被忽略。

### 文件夹

- `code`：包括你的ipynb文件，Section2代码和一切有必要的说明文件

## 测试方法

### `result `：按正确率给分，代码如下

```python
if accuracy >= 0.9:
    score = 80
elif acc >= 0.7:
    score = math.ceil(80 * (acc - 0.7) / 0.2)
else:
    score = 0
```

## `submission.csv`

数据共240行，第一行将被省略

```python
correct_num = check_correct()
score = min(100, ceil(100.0/240*(correct_num * 1.1)))
```