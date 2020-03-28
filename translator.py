import re
import csv
from unidecode import unidecode

# The translator
class Translator:
  class Graph:
    def __init__(self, leftSize, rightSize):
      self.buffPos = 0
      self.adj = [[None] * (leftSize + 1), [None] * (rightSize + 1)]
      self.staticList = [None]
      pass
    
    def addEdge(self, u, v):
      self.buffPos += 1
      self.staticList.append((v, self.adj[0][u]))
      self.adj[0][u] = self.buffPos
      
      self.buffPos += 1
      self.staticList.append((u, self.adj[1][v]))
      self.adj[1][v] = self.buffPos

    def neighbors(self, side, u):
      pos = self.adj[side][u]
      ret = []
      while pos:
        ret.append(self.staticList[pos][0])
        pos = self.staticList[pos][1]
      return ret

  def __init__(self):
    self.invalid = {"", "g", "mg", "mcg", "ml", "mikrogramm", "unit"}

    leftSize = sum(1 for line in open('meds/en_meds.csv', "r"))
    rightSize = sum(1 for line in open('meds/de_meds.csv', "r"))
    
    # Compute the graph
    self.graph = self.Graph(leftSize, rightSize)
    with open("matcher/graph.matched", "r") as input:
      for row in input:
        args = row.split()
        u = int(args[0])
        for v in args[1:]:
          self.graph.addEdge(u, int(v))
          
    # Store the medicines (en, de)
    self.medIndex = [dict(), dict()]
    self.meds = [list(), list()]
    self.rawMeds = [list(), list()]
         
    # Parse the english medicines
    with open("drugbank_vocabulary.csv", "r") as input:
      csvreader = csv.reader(input)
     
      next(csvreader)
      rowIndex = 0
      for row in csvreader:
        commonName = unidecode(row[2])
        splitted = self.prepare(commonName, side=0)
        for part in splitted:
          part = part.lower()
          if not part in self.medIndex[0]:
            self.medIndex[0][part] = [rowIndex]
          else:
            self.medIndex[0][part].append(rowIndex)
        self.meds[0].append(" ".join(splitted))
        self.rawMeds[0].append(row[0])
        rowIndex += 1
        
    # Parse the german medicines
    with open("meds/de_meds.csv", "r") as input:
      rowIndex = 0
      for row in input:
        row = row.strip()
        splitted = self.prepare(row, side=1)
        for part in splitted:
          part = part.lower()
          if not part in self.medIndex[1]:
            self.medIndex[1][part] = [rowIndex]
          else:
            self.medIndex[1][part].append(rowIndex)
        self.meds[1].append(" ".join(splitted))
        self.rawMeds[1].append(row)
        rowIndex += 1
    pass
  
  def prepare(self, medicine, side):
    # Split an english medicine
    if side == 0:
      return re.split("[\\s,/'`-]+", medicine)
    elif side == 1:
      splitted = re.split("[-_]+", medicine)
      splitted.pop()
      return splitted
      ret = []
      for part in splitted:
        if part.isdigit():
          continue
        part = re.sub(r"[0-9]+", "", part) 
        if part in self.invalid:
          continue
        ret.append(part)
      return ret
  
  def query(self, targetLanguage, medicine):
    if not medicine:
      return ""
    medicine = unidecode(medicine.strip()).lower()
    splitted = re.split("[\\s,/-]+", medicine)
    side = int(targetLanguage == "English")
    if len(splitted) == 1:
      single = splitted[0]
      if not single in self.medIndex[side]:
        return []
      all = dict()
      for index in self.medIndex[side][single]:
        neighbors = self.graph.neighbors(side, index)
        for v in neighbors:
          if v in all:
            all[v] += 1
          else:
            all[v] = 1
      maxCount = 0
      best = []
      for index, count in all.items():
        if count > maxCount:
          best = [index]
          maxCount = count
        elif count == maxCount:
          best.append(index)
      return [[self.meds[int(not side)][elem] for elem in best], [self.rawMeds[int(not side)][elem] for elem in best]]
    else:
      all = dict()
      found = 0
      for part in splitted:
        if not part in self.medIndex[side]:
          continue
        found += 1
        seen = set()
        for index in self.medIndex[side][part]:
          neighbors = self.graph.neighbors(side, index)
          for v in neighbors:
            if v in seen:
              continue
            seen.add(v)
            if v in all:
              all[v] += 1
            else:
              all[v] = 1
      lowerBound = len(splitted) / 2 + int((len(splitted) % 2 == 1) or (len(splitted) == 2))
      if found < lowerBound:
        return []
      maxCount = 0
      best = []
      for index, count in all.items():
        if count > maxCount:
          best = [index]
          maxCount = count
        elif count == maxCount:
          best.append(index)
      if maxCount < lowerBound:
        return []
      return [[self.meds[int(not side)][elem] for elem in best], [self.rawMeds[int(not side)][elem] for elem in best]]
  pass
