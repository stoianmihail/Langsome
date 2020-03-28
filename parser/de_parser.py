# Implementation of the parser of "Gelbe Seiten" for medicines
import os
import html
import urllib.request
import sys
import string
from unidecode import unidecode

def getHtmlText(url):
# get the html text from this url
  # Connect to the site
  request = urllib.request.Request(url)
  response = urllib.request.urlopen(request)
    
  # Decode the site into utf-8
  return response.read().decode('utf-8', errors = 'backslashreplace')

def computeNumberOfPages(char):
# Compute how many pages in the website of medicines, which begin with char, has
  initialUrl = "https://www.gelbe-liste.de/produkte/" + char
  text = getHtmlText(initialUrl)
  
  # Zero medicines?
  notFoundPattern = "<p>0 Medikamente und pharmazeutische Produkte, die mit " + char + " beginnen, in der Datenbank gefunden.</p>"
  pos = text.find(notFoundPattern)
  
  # If not, then search for the maximum page number
  if pos == -1:
    pagePattern = "<a href=\"/produkte/" + char + "?page="
    pos = text.find(pagePattern)
    
    maxPage = 0
    while pos != -1:
      ptr = pos + len(pagePattern)
      pageNo = int(text[ptr : (ptr + text[ptr:].find('"'))])
      if pageNo > maxPage:
        maxPage = pageNo
      # Continue with the next occurence
      text = text[ptr:]
      pos = text.find(pagePattern)
    # And return the maximum value
    return max(1, maxPage)
  else:
    return 0
  
def extractLinksFromPage(char, page):
# Extract the links from the current page
  pageUrl = "https://www.gelbe-liste.de/produkte/" + char + "?page=" + str(page)
  text = getHtmlText(pageUrl)
  dataBeginPattern = "<ul class=\"product-list\">"
  dataEndPattern = "</ul>"
  
  beginPos = text.find(dataBeginPattern)
  text = text[beginPos:]
  endPos = text.find(dataEndPattern)
  text = text[:endPos]
  
  # And extract the links
  links = []
  linkPattern = "<a href=" + "\"/produkte/"
  pos = text.find(linkPattern)
  while pos != -1:
    offset = pos + len(linkPattern)
    links.append(text[offset:(offset + text[offset:].find('"'))])
    text = text[offset:]
    pos = text.find(linkPattern)
  return links

# Return which files are not empty at the end       
def extractCorr():
  startWith = [chr(x) for x in range(ord('0'), ord('9') + 1)] + [chr(x) for x in range(ord('A'), ord('Z') + 1)]
  
  line = 1
  corr = []
  for char in startWith:
    print("Parse " + char + ": progess=[" + "{0:.2f}".format(float(line / len(startWith)) * 100) + "%]")
    
    pageCount = computeNumberOfPages(char)
    if pageCount != 0:
      corr.append((char, pageCount))
    line += 1
  return corr
  
# Parse all german medicines
def extractAllMedRefs(corr):
  count = 1
  for (char, pageCount) in corr:
    output = open("meds/refs/list_" + char, "w")
  
    print("Parse links of " + char + ": progess=[" + "{0:.2f}".format(float(count / len(corr)) * 100) + "%]")
    
    curr = []
    for page in range(1, pageCount + 1):
      curr += extractLinksFromPage(char, page)
    output.write(str(pageCount) + '\n')
    for ref in curr:
      output.write(ref + '\n')
    output.close()
    count += 1
  pass

# Merge all the parsed files (this can be also done even after the files have been generated)
def merge() -> None:
  output = open("meds/de_meds.csv", "w")
  for file in sorted(os.listdir("meds/refs")):
    with open("meds/refs/" + file, "r") as input:
      next(input)
      for line in input:
        output.write(line)
  output.close()

# Check if there exists at least one letter the medicines of which have been parsed
def isAlreadyAvailable() -> bool:
  possible = [chr(x) for x in range(ord('0'), ord('9') + 1)] + [chr(x) for x in range(ord('A'), ord('Z') + 1)]
  
  if not os.path.isdir("meds/refs"):
    return False
  
  # Check that at list one list exists
  for char in possible:
    if os.path.isfile("meds/refs/list_" + char):
      return True
  return False

def main():
  # First check if the directory is not empty
  available = isAlreadyAvailable()
  if available:
    merge()
  else:
    extractAllMedRefs(extractCorr())
    merge()
  pass
  
if __name__ == '__main__':
  main()
