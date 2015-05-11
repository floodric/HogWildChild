#!/usr/bin/env python3

import xml.etree.ElementTree as et
import time
#import ngrams
import struct
import random

german = 'doutput.xml'
gVal = -1
portugese = 'poutput.xml'
pVal = 1

path = '../data/'
baseType = '{http://www.mediawiki.org/xml/export-0.9/}'
pageType = baseType + 'page'
revisionType = baseType + 'revision'
textType = baseType + 'text'

# input: tree:xml.etree.ElementTree 
# output: list: pageContent:text
def getSamples(tree):
  root = tree.getroot()
  pages = root.findall(pageType)
  revisions = map(lambda page: page.find(revisionType), pages)
  textElems = map(lambda rev: rev.find(textType), revisions)
  textStrings = map(lambda t: t.text, textElems)
  return list(textStrings)

# input: filename to read
# output: list: pageContent:text
def readWiki(filename):
  readStart = time.clock()
  tree = et.parse(path+filename)
  readTime = time.clock() - readStart

  parseStart = time.clock()
  samples = getSamples(tree)
  parseTime = time.clock() - parseStart
  print("{} pages in {}".format(len(samples),filename) )

  print("reading took {}, parsing {}".format(readTime,parseTime))
  return samples

######################
### getting ngrams functions
#######################
# ngram table: 
# ngram:string -> count:int
# eg
# {'aaa':1,
#  'elo':5,
#  '<japanese characters>':1,
# }

#ngrams.gramSize = 3
gramSize = 3
# input: word:string
# output: ngram table (see above)
def getGrams(word):
  ngrams = {}
  for i in range(len(word) - (gramSize -1)):
    ngrams.setdefault(word[i:i+gramSize],0)
    ngrams[word[i:i+gramSize]] += 1
  return ngrams

# input: sample:string (large text)
# output: ngram table (see above)
def sampleGrams(sample):
  wordList = sample.split()
  wordGrams = map(getGrams, wordList)

  grams = {}
  for wordGram in wordGrams:
    for ngram,count in wordGram.items():
      grams.setdefault(ngram,0)
      grams[ngram] += count
    
  return grams

# converts a dictionary of
# input: ngram table (see above)
#        lookupTable: ngram:string -> ngram_index:int
# output: dict: ngram_index:int -> count:int
def convertToVector(gramCountDict, lookupTable):
  vec = {}
  totalCount = sum(list(gramCountDict.values()))
  for ngram,count in gramCountDict.items():

    vecCol = lookupTable[ngram]
    vec[vecCol] = (1.0*count)/totalCount
    
  return vec


###################
### doing useful work
##################

gsamp = readWiki(german)
psamp = readWiki(portugese)

startgrams = time.clock()
gsampGrams = list(map(sampleGrams,gsamp))
psampGrams = list(map(sampleGrams,psamp))
gramsTime = time.clock() - startgrams 
print("getting grams took {} time".format(gramsTime) )

# combine all the 3-grams of all samples into a master set
allGrams = set()
for sample in (gsampGrams + psampGrams):
  allGrams |= set(sample.keys())

vecstart = time.clock()
# now convert it into an ordered list for vector indexing
gramsLookup = sorted(list(allGrams))
# this is weird, but dictionary of ngram->index in sorted list (quicker lookup)
# make tuple pairs, then cast the new array as a dict
gramsLookupD = dict([ (gramsLookup[i],i) for i in range(len(gramsLookup)) ] )

gVecs = list(map(lambda x: convertToVector(x,gramsLookupD), gsampGrams))
pVecs = list(map(lambda x: convertToVector(x,gramsLookupD), psampGrams))

# add the -1 column (vector identifier) to the specified value
for vec in gVecs:
  vec[-1] = gVal
for vec in pVecs:
  vec[-1] = pVal

vectime = time.clock() - vecstart
print(vectime, " time to convert to vectors")

# can combine lists since they are now classified
allVecs = gVecs + pVecs

rowCount = 1
def structify(vec):
  global rowCount
  records = []
  # our code expects the columns to be sorted in ascending order
  for ngramIndex in sorted(list(vec.keys())):
    records.append(struct.pack("iid",rowCount,ngramIndex,vec[ngramIndex]))
  rowCount +=1
  return records

def writeBinaryData(vectorList,fname,testPercent=0.1):
  testPortion = int(testPercent*len(vectorList))
  filename = "{}.binary.{}.dat"

  random.shuffle(vectorList)

  # turn each vector into a binary struct of itself
  structList = list(map(structify,vectorList))

  # write first chunk into testing file
  testFile = open(filename.format(fname,"train") ,"wb")
  # write number of records we are going to see
  testRecords = sum(list(map(lambda x: len(x), structList[:testPortion]))) 
  testFile.write(struct.pack("i",testRecords))

  print("writing {} into training data".format(len(structList[:testPortion])))
  for vecStructList in structList[:testPortion]:
    for vecStruct in vecStructList: 
      testFile.write(vecStruct) # write each vector
  testFile.close()
# write second chunk into training file
  trainFile = open(filename.format(fname,"test") ,"wb")
  # write number of records we are going to see
  trainRecords = sum(list(map(lambda x: len(x), structList[testPortion:]))) 
  trainFile.write(struct.pack("i",trainRecords))
  print("writing {} into test data".format(len(structList[testPortion:])))
  for vecStructList in structList[testPortion:]: 
    for vecStruct in vecStructList: 
      trainFile.write(vecStruct) # write each vector
  trainFile.close()
  
  return

writeBinaryData(allVecs,"wiki")
