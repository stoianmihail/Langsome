# Parser for the open data from drugbank (drugbank_vocabulary)
import re
import sys
import csv
import urllib.request
from unidecode import unidecode

def getHtmlText(url):
  # Connect to the site
  request = urllib.request.Request(url)
  response = urllib.request.urlopen(request)

  # Decode the site into utf-8
  return response.read().decode('utf-8', errors = 'backslashreplace')

def parsePrices(id):
  # Parse also the medicines which are listed under 'Prices'
  url = "https://www.drugbank.ca/drugs/" + id
  text = getHtmlText(url)
  
  medicines = []
  pos = text.find("Prices")
  if pos == -1:
    return medicines
  
  text = text[(pos + len("Prices")):]

  # Check if the field of prices exists
  check = "<span class='not-available'>Not Available</span>"  
  if text.find(check) != -1 and text.find(check) <= 50:
    return medicines

  # And read the table
  beginPtr = text.find("<tbody>")
  endPtr = beginPtr + text[beginPtr:].find("</tbody>")
  
  # Read each row from the table
  text = text[beginPtr : endPtr]
  pos = text.find("<tr>")
  while pos != -1:
    currEnd = pos + text[pos:].find("</tr>")
    if currEnd == -1:
      print("error while parsing prices")
      sys.exit(1)
    row = text[pos : currEnd]
    beginCol = row.find("<td>")
    if beginCol == -1:
      print("no medicine found while parsing prices")
      sys.exit(1)
    endCol = beginCol + row[beginCol:].find("</td>")
    
    # Extract the medicine
    medicine = row[(beginCol + len("<td>")) : endCol]
    medicines.append(medicine)
    
    # And go to the next medicine
    text = text[(currEnd + len("</tr>")):]
    pos = text.find("<tr>")
  return medicines

def cleanMedicines(medicines):
  # Clean the medicines which have been read from 'Prices'
  final = []
  for medicine in medicines:
    compound = ""
    splitted = re.split("[\s/]+", medicine)
    for index, part in enumerate(splitted):
      # Analyze the part of the medicine
      if part.find("&") != -1 and part.find(";") != -1:
        seq = part[part.find("&") : (part.find(";") + 1)]
        part = part.replace(seq, "")
      if part:
        compound += part + " " * int(index != len(splitted) - 1)
    if compound:
      final.append(unidecode(compound))
  return final

def parseAllMedicines(en_file):
  # From the drugbank .csv file keep only the common name, the synonyms and the prices
  # Note that each synonym which comes from the 'Prices' field is sticked to its bar (this is how we differentiate in matcher)
  with open(en_file, "r") as inputFile:
    with open("meds/en_meds.csv", "w") as outputFile:
      csvreader = csv.reader(inputFile)
      csvwriter = csv.writer(outputFile)
      
      next(csvreader)
      for row in csvreader:
        id = row[0]
        print("Parsing for " + str(id))
        commonName = unidecode(row[2])
        synonyms = unidecode(row[5])
        prices = cleanMedicines(parsePrices(id))
        if prices:
          csvwriter.writerow([commonName, synonyms + "|" + "|".join(prices)])
        else:
          csvwriter.writerow([commonName, synonyms])
  pass

def main(en_file):
  # python3 <script> <drugbank_vocabulary>
  parseAllMedicines(en_file)
  pass
  
if __name__ == '__main__':
  main(sys.argv[1])
