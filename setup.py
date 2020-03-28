import os

def main():
  requirements = ['unidecode']

  for requirement in requirements:
    os.system("pip install " + requirement)
    
if __name__ == '__main__':
  main()
