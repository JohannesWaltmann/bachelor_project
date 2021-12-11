#!/usr/bin/env python
# coding: utf-8

# In[1]:


import math
from scipy.io import wavfile
from scipy.fft import fft
import matplotlib.pyplot as plt
import numpy as np
import os

plt.rcParams['figure.facecolor'] = 'white'  # enhances visibility in darkmode IDEs
plt.rcParams["figure.figsize"] = (10, 10)


# In[2]:


DATAPATHS = []
rootdir = './bachelor_project/Data/Study Recordings'

for subdir, dirs, files in os.walk(rootdir):
    for file in files:
        path = os.path.join(subdir, file)
        DATAPATHS.append(path)


# In[4]:


DATASET = []



for i in range(len(DATAPATHS)):
    samplerate, data = wavfile.read(DATAPATHS[i])
    _tuple = (samplerate, np.asarray(data))
    DATASET.append(_tuple)

print(DATASET[35][1])


# In[5]:


def fourier_transform(data):
    transformed_data = fft(data)
    return np.abs(transformed_data[:int((len(transformed_data)/2)-1)])


# In[8]:



fix = math.floor(18/3)
fiy = 3

fig, axs = plt.subplots(fix, fiy, figsize = (3*fiy, 3*fix))
for i, j in zip(range(0, 18), range(91, 108)):
    print(i, j)
    try:
        temp = fourier_transform(DATASET[j][1])
        axs[int(i/3)][i%3].plot(temp)
    except Exception as e:
        print(str(e))
        continue

plt.savefig('p2_s1_fft.png')


# In[12]:


print(fourier_transform(DATASET[j][1]))

print(fourier_transform(DATASET[107][1]))

