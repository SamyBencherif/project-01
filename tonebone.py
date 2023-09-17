import numpy as np
from scipy.io import wavfile
import math
import random
import subprocess
from time import sleep

# Additive synthesis

sampleRate = 44100

HS = 2**(1/12.)
OCT = 2

def sumvec(*k):
  if len(k) <= 0:
    print("What the hell are you doing?")
    return
  elif len(k) == 1:
    return k[0]
  elif len(k) == 2:
    A,B = k
    v = []
    common = min(len(A), len(B))
    for i in range(common):
      v.append(A[i]+B[i])
    v.extend(A[common:])
    v.extend(B[common:])
    return np.array(v)
  else:
    return sumvec(sumvec(k[0], k[1]), *k[2:])

def addsyn(instrument=[100], fund_freq=440, duration=1, volume=100):
  # don't let waveform definition influence volume
  if sum(instrument):
    volume /= sum(instrument)
  t = np.linspace(0, math.ceil(duration), sampleRate * math.ceil(duration))
  # handle non-integer duration in seconds
  t = t[:int(len(t) * duration/math.ceil(duration))]
  print("ADDITIVE: " + str(instrument)[1:-1])
  media = np.zeros(len(t))
  for i in range(len(instrument)):
    media += volume*instrument[i]/100.*np.sin((i+1) * fund_freq * 2 * np.pi * t)
  return media

def playwrite(media):
  media = np.array(media)
  wavfile.write('Zone.wav', sampleRate, media)
  subprocess.call('mpv Zone.wav', shell=True)


# pieces are in caps
def ZONE():
  # randomly generate instrument
  #for i in range(5):
  #  instrument.append(random.randint(-100,100))

  # random variance
  # for i in range(len(instrument)):
  #   instrument[i] = instrument[i] + random.randint(-30, 30)

  f = 440/OCT**2/HS**7

  doot = [-70, -17, -97, -4, -74 ]

  #note(doot, f)

  # iterations on buzz
  # note([-89, -23, 23, 11, -23, 0, 0, 0], f)
  # note([-96, -46, 38, 17, -34, -1, 6, -12], f)
  # buzz = [-124, -18, 47, -12, -50, 14, 34, -27]
  # note(buzz, f)

  f = 440/OCT**2/HS**7

  # combine instruments A and B to make C
  A = [-136, 9, 77, -38, -39, 39, 44, -9]
  B = [-117, -33, 55, -29, -66, -3, 4, -6]
  C = sumvec(A,B)
  Z = [0] # silent instrument

  # play 5, 9, 3 followed by 4 and 5 simulaneously (note = D2-x)
  S = []
  #S.extend(addsyn(C, f/HS**5, 2))
  #S.extend(addsyn(C, f/HS**9, 2))
  #S.extend(addsyn(C, f/HS**3, 2))
  #S.extend(addsyn(C, f/HS**5, 2))

  #S.extend(addsyn(Z, .3))

  #S.extend(addsyn(C, f/HS**5, 2))
  #S.extend(addsyn(C, f/HS**9, 2))
  #S.extend(addsyn(C, f/HS**4, 2))
  #S.extend(addsyn(C, f/HS**5, 2))

  for i in range(4):
    S.extend(addsyn(C, f*OCT**1/HS**(5-i), .5))

  S.extend(addsyn(C, f/HS**5, 2)) 

  for i in range(4):
    S.extend(addsyn(C, f*OCT**1/HS**(4-i), .5))

  S.extend(addsyn(C, f/HS**9, 2)) 

  for i in range(3,-1,-1):
    S.extend(addsyn(C, f*OCT**1/HS**(5-i), .5))

  S.extend(addsyn(C, f/HS**3, 2)) 

  for i in range(4):
    S.extend(addsyn(C, f*OCT**1/HS**(8-i), .5))

  S.extend(addsyn(C, f/HS**5, 2)) 

  for i in range(4):
    S.extend(addsyn(C, f*OCT**1/HS**(5-i), .5))

  S.extend(addsyn(C, f/HS**5, 2)) 

  for i in range(4): 
    S.extend(addsyn(C, f/HS**(9+i), .5)) 

  S.extend(addsyn(C, f/HS**3, 2)) 
  S.extend(addsyn(C, f/HS**-11, 2)) 

  playwrite(S)

  # random variation
  #for i in range(len(buzz)):
  #  buzz[i] = buzz[i] + random.randint(-30, 30)
  #note(buzz, f)

ZONE()
