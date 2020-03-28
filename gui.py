from tkinter import *
from translator import Translator
import re
import webbrowser
import platform
from PIL import ImageTk, Image

class GUI:
  def __init__(self, language1, language2):
    self.translator = Translator()
    self.default = language1
    self.choices = {language1, language2}
    self.input = None
    self.output = None
    self.hyperlinkPattern = re.compile(r'<a href="(?P<address>.*?)">(?P<title>.*?)</a>')
    
  def _formatHyperLink(self, message):
    text = self.output
    start = 0
    for index, match in enumerate(self.hyperlinkPattern.finditer(message)):
      groups = match.groupdict()
      text.insert("end", message[start: match.start()])
            
      # Insert hyperlink tag here
      text.insert("end", groups['title'])
      text.tag_add(str(index), "insert-%d chars" % len(groups['title']),"insert",)
      text.tag_config(str(index), foreground="blue", underline=1)
      text.tag_bind(str(index), "<Enter>", lambda *a, **k: text.config(cursor="hand2"))
      text.tag_bind(str(index), "<Leave>", lambda *a, **k: text.config(cursor="arrow"))
      text.tag_bind(str(index), "<Button-1>", self._callbackFactory(groups['address']))
      start = match.end()
    else:
      text.insert("end", message[start:])

  def _callbackFactory(self, url):
    return lambda *args, **kwargs: webbrowser.open(url)
   
  # The actual translation
  def translate(self, even = None):
    fromLanguage = self.language.get()
    best = self.translator.query(fromLanguage, self.input.get())
    self.output.delete('1.0', END)
    if not best:
      self._formatHyperLink("No matching medicine in our database.\nPlease check the target language or the spelling.\n")
    else:
      pattern = ""
      commonNames = best[0]
      rawRefs = best[1]
      if len(commonNames) == 1:
        pattern = "Result"
      else:
        pattern = "Results"
      urlPattern = "https://www.drugbank.ca/drugs/" if fromLanguage == "German" else "https://www.gelbe-liste.de/produkte/"
      size = len(commonNames)
      self.output.insert(END, str(size) + " " + pattern + "\n" + ("-" * (1 + len(str(size)) + len(pattern))) + "\n")
      for index, elem in enumerate(commonNames):
        self._formatHyperLink(str(index + 1) + ". <a href=\"" + urlPattern + rawRefs[index] + "\">" + elem + "</a>\n")
      
  def func(self, event):
    pass
    
  def callback(self, event):
    self.root.after(10, self.selectAll, event.widget)
    
  def selectAll(self, widget):
    widget.select_range(0, 'end')
    widget.icursor('end')

  def menuEvent(self, event):
    self.output.delete('1.0', END)

  def run(self):
    # Tkinter root window with title
    self.root = Tk()
    
    # Check for operating system
    if platform.system() == "Linux":
      self.root.attributes('-zoomed', True)
    elif platform.system() == "Windows":
      self.root.state('zoomed')
    else:
      print("Operating System not supported")
      sys.exit(1)
    self.root.title("LangMed: Interlingual Medicine Translator")

    # Set color
    mainFrame = Frame(self.root, bg='bisque')
    mainFrame.grid(column=0, row=0, sticky="nsew")
    
    # Configurations
    mainFrame.columnconfigure(1, weight=1)
    mainFrame.columnconfigure(1, pad=1)
    mainFrame.rowconfigure(0, weight=0)
    mainFrame.rowconfigure(1, pad=1)
    mainFrame.pack(fill="both", expand=True)

    # Languages for option menu
    self.language = StringVar(self.root)
    self.language.set(self.default)

    # Create drop-down and arrane the grid
    languageMenu = OptionMenu(mainFrame, self.language, *(self.choices), command=self.menuEvent)
    languageMenu.config(font=("Courier", 16))
    menu = self.root.nametowidget(languageMenu.menuname)
    menu.config(font=("Courier", 16))
    Label(mainFrame, text="Target language", font=("Courier", 16), borderwidth=2, relief="groove").grid(row=0, column=0, pady=20, padx=20, sticky = W+S+E+N)
    languageMenu.grid(row=0, column=1, padx=20, pady=20, sticky="ws")

    # Text Box to take user input
    inputLabel = Label(mainFrame, text = "Enter medicine", font=("Courier", 16), borderwidth=2, relief="groove").grid(row=1, column=0, padx=20, pady=20, sticky = N+S+E+W)
    self.input = StringVar()
    inputBox = Entry(mainFrame, textvariable=self.input,font=("Courier", 16))
    inputBox.bind('<Control-a>', self.callback)
    inputBox.grid(row=1, column=1, padx=20, pady=20, stick="nsew")

    # Create a button to call Translator function and bind to Enter
    self.root.bind('<Return>', self.translate)
    button = Button(mainFrame, text='Translate', command=self.translate, font=("Courier", 16)).grid(row=2, column=1, columnspan=2, padx=20, pady=20, sticky="w")

    # Add the scrolling bar
    textScroll = Scrollbar(mainFrame)
    textScroll.grid(row=3, column=2, padx=10, pady=20, sticky="nsew")
    
    # Text Box to output the translation
    outputLabel = Label(mainFrame, text = "Translation", font=("Courier", 16), borderwidth=2, relief="groove").grid(row=3, column=0, padx=20, pady=10, sticky="new")
    self.output = Text(mainFrame, font=("Courier", 16), yscrollcommand=textScroll.set)
    self.output.grid(row=3, column=1, padx=20, pady=10, sticky="nsew")
    
    # And set the scrolling bar
    textScroll.config(command=self.output.yview)

    # Add the icon
    img = ImageTk.PhotoImage(Image.open("util/small_icon.png").resize((150, 150)))
    panel = Label(mainFrame, image=img)
    panel.grid(row=4, column=0, columnspan=4, padx=30, pady=5, sticky="ns")

    # Recursion
    self.root.mainloop()
    
# And run
gui = GUI("English", "German")
gui.run()
