import os
import sys
from os.path import expanduser

def createDesktopIcon():
  home = expanduser("~")
  if not os.path.exists(home + "/Desktop/Langsome.desktop"):
    file = open(home + "/Desktop/Langsome.desktop", "w")
    pythonPath = sys.executable
    
    fileContent = "[Desktop Entry]\n"
    fileContent += "Name=Langsome\n"
    fileContent += "Type=Application\n"
    fileContent += "Comment=Interlingual Medicine Translator\n"
    fileContent += "Terminal=false\n"
    fileContent += "Icon=" + os.getcwd() + "/util/langsome_icon.png\n"
    fileContent += "Exec=" + pythonPath + " " + os.getcwd() + "/gui.py\n"
    
    file.write(fileContent)
  os.system("chmod u+x " + home + "/Desktop/Langsome.desktop")
  pass

if __name__ == '__main__':
  createDesktopIcon()


