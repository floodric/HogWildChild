import time
import struct
import multiprocessing
#import unicodedata as ud

german = 'doutput.xml'
gVal = -1
portugese = 'poutput-big.xml'
pVal = 1
path = "../data/"

openText = "<text xml:space=\"preserve\">"
closeText = "</text>"

parsedSamples = []
allGrams = set()
lookupTable = {} # dict([ (sortedGrams[i],i) for i in range(len(gramsLookup)) ] )

def combineGramSets(gramSet,num):
  global allGrams
  allGrams |= gramSet

def combineVectors(sample,num):
  global parsedSamples
  parsedSamples.append(structify(sample))

def getVectors(sample,label):
  ngrams = sampleGrams(sample)
  vec = convertToVector(ngrams,label)
  return vec

printCount = 0
def reduceList(resList,accFun):
  numFinished = 0
  global printCount
  toDel = []
  startlen = len(resList)
  highestSampleNum = -1
  for i in range(len(resList)):
    if resList[i][0].ready():

      accFun(resList[i][0].get(),resList[i][1])
      highestSampleNum = max(highestSampleNum,resList[i][1])
      toDel.append(i)

  
  printCount+=1
  if printCount % 5 == 0:
    print("finished {}, {}".format(len(toDel),highestSampleNum))

  # clean up list, deleting done elements && account for list index "drift" leftwards
  offset = 0
  for i in toDel: 
    del resList[i-offset]
    offset += 1
          
  assert(len(resList) == (startlen - len(toDel)))
  return


def parseAndRunSamples(fname,sampleFun,accFun,label,batchTrigger=1000):
  f = open(path+fname,"rU")
  sample = []
  asyncRes = []
  readingText = False
  pool = multiprocessing.Pool(32)
  
  batchCount = 0

  sampleNum = 0
  for line in f:
    if readingText and closeText in line:
      line = line[:line.find(closeText)] # grab up to the point where we close
      readingText = False
      textSamp = ''.join(sample)
      batchCount += 1
      sampleNum += 1
      if label != 0:
        asyncRes.append([pool.apply_async(sampleFun,[textSamp,label]),sampleNum])
      else:
        asyncRes.append([pool.apply_async(sampleFun,[textSamp]),sampleNum])
        

      if batchCount % batchTrigger == 0:
        batchCount = 0
        reduceList(asyncRes,accFun)
                    
      sample = []

    elif (not readingText) and openText in line:  
      readingText = True
      line = line[line.find(openText):] # grab after we open
      sample.append(line)
  
    elif readingText:
      sample.append(line)
  f.close()

  print("\n\n!!! done reading file !!!\n\n")
  print(len(asyncRes), "jobs left after batching")
  pool.close()
  pool.join()
  print("all workers completed")
  for res in asyncRes:
    accFun(res[0].get(),res[1])
  
  return
######################
### getting ngrams functions
#######################
# ngram table: 
# ngram:string -> count:int
# eg
# {'aaa':1,
#  'elo':5,
#  '「ノイ':1,
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

def getGramsSet(word):
  ngrams = set()
  for i in range(len(word) - (gramSize -1)):
    ngrams.add(word[i:i+gramSize])
  return ngrams

#see 5.7.1 General Category Values in http://www.unicode.org/reports/tr44/tr44-6.html
# input: sample:string (large text)
# output: ngram table (see above)
def sampleGrams(sample):
  global blackList
  blackList = [
    "Nd","Pd","Po","Ps","Pc","Pi","Pe","Pf","Lo"
  ]
  import unicodedata as ud
  dataTypes = map(lambda x: [ud.category(x),x], sample)
  cleaning = filter(lambda x: (x[0] not in blackList), dataTypes)
  cleaned = list(map(lambda x: x[1], cleaning))
  cleanSamp = ''.join(cleaned)
  wordList =  cleanSamp.split() 
  wordGrams = map(getGrams, wordList)

  grams = {}
  for wordGram in wordGrams:
    for ngram,count in wordGram.items():
      grams.setdefault(ngram,0)
      grams[ngram] += count
    
  return grams

def sampleGramsSet(sample):
  blackList = [
    "Nd","Pd","Po","Ps","Pc","Pi","Pe","Pf","Lo"
  ]
  import unicodedata as ud
  dataTypes = map(lambda x: [ud.category(x),x], sample)
  cleaning = filter(lambda x: (x[0] not in blackList), dataTypes)
  cleaned = list(map(lambda x: x[1], cleaning))
  cleanSamp = ''.join(cleaned)
  wordList =  cleanSamp.split()
  wordGramsSets = map(getGramsSet, wordList)

  grams = set()
  for wordGramSet in wordGramsSets:
    grams |= wordGramSet
 
  return grams

# converts a dictionary of
# input: ngram table (see above)
#        lookupTable: ngram:string -> ngram_index:int
# output: dict: ngram_index:int -> count:int
def convertToVector(gramCountDict,label):
  global lookupTable 
  vec = {}
  totalCount = sum(list(gramCountDict.values()))
  for ngram,count in gramCountDict.items():

    vecCol = lookupTable[ngram]
    vec[vecCol] = (1.0*count)/totalCount
    
  vec[-1] = label
  return vec

rowCount = 1
def structify(vec):
  global rowCount
  records = []
  for k,v in vec.items():
    records.append(struct.pack("iid",rowCount,k,v))
  rowCount+=1
  return records

def writeBinaryData(fname,testPercent=0.1):
  global parsedSamples
  testPortion = int(testPercent*len(parsedSamples))
  filename = "{}.binary.{}.dat"

  print("writing")
  # turn each vector into a binary struct of itself

  # write first chunk into training file
  trainFile = open(filename.format(fname,"train") ,"wb")
  trainFile.write(struct.pack("i",len(parsedSamples[:testPortion]))) # write number of vectors we are going to see
  print("writing {} into training data".format(len(parsedSamples[:testPortion])))
  for vecStructList in parsedSamples[:testPortion]:
    for vecStruct in vecStructList: 
      trainFile.write(vecStruct) # write each vector
  trainFile.close()

  # write second chunk into testing file
  testFile = open(filename.format(fname,"test") ,"wb")
  testFile.write(struct.pack("i",len(parsedSamples[testPortion:])))
  print("writing {} into test data".format(len(parsedSamples[testPortion:])))
  for vecStructList in parsedSamples[testPortion:]: 
    for vecStruct in vecStructList: 
      testFile.write(vecStruct) # write each vector
  testFile.close()
  
  return

### useful work
def run():
  global lookupTable
  global allGrams

  # get the beginner list
  print("parsing file")
  parseAndRunSamples(portugese,sampleGramsSet,combineGramSets,0,batchTrigger=100000)
  parseAndRunSamples(german,sampleGramsSet,combineGramSets,0,batchTrigger=100000)
  
  sortedGrams = sorted(list(allGrams))
  lookupTable = dict( [ (sortedGrams[i],i) for i in range(len(sortedGrams)) ] )
  print("finished table")

  del allGrams

  parseAndRunSamples(portugese,getVectors,combineVectors,pVal,batchTrigger=10000)
  parseAndRunSamples(german,getVectors,combineVectors,gVal,batchTrigger=10000)
  del lookupTable

  writeBinaryData("pout")
