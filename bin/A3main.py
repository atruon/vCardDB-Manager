#!/usr/bin/env python3
from ast import FunctionType
from ctypes import *
from ctypes.wintypes import POINT
from time import timezone
from unittest import result
import mysql.connector
from datetime import datetime


from asciimatics.widgets import Frame, ListBox, Layout, Divider, Text, \
    Button, TextBox, Widget, Label
from asciimatics.scene import Scene
from asciimatics.screen import Screen
from asciimatics.exceptions import ResizeScreenError, NextScene, StopApplication
import sys 
import os

sharedLibPath = "libvcparser.so"
fileDir = "./cards"
cardsDir = "./cards/"
filesList = os.listdir(fileDir)

vcParserLib = CDLL(sharedLibPath)

# ---------------- C Structs START-------------------------
class Node (Structure):
    pass
Node._fields_ = [("data", c_void_p),
                 ("previous", POINTER(Node)),
                 ("next", POINTER(Node))]

deleteDataType = CFUNCTYPE(None, c_void_p)
compareType = CFUNCTYPE(c_int, c_void_p, c_void_p)
printDataType = CFUNCTYPE(c_char_p, c_void_p)

class List (Structure):
    _fields_ = [("head", POINTER(Node)),
                ("tail", POINTER(Node)),
                ("length", c_int),
                ("deleteData", deleteDataType),
                ("compare", compareType),
                ("printData", printDataType)]

class DateTime (Structure):
    _fields_ = [("UTC", c_bool),
                ("isText", c_bool),
                ("date", c_char_p),
                ("time", c_char_p),
                ("text", c_char_p)]

class Parameter (Structure):
    _fields_ = [("name", c_char_p),
                ("value", c_char_p)]

class Property (Structure):
    _fields_ = [("name", c_char_p),
                ("group", c_char_p),
                ("parameters", POINTER(List)),
                ("values", POINTER(List))]

class vCard (Structure):
    _fields_ = [("fn", POINTER(Property)),
                ("optionalProperties", POINTER(List)),
                ("birthday", POINTER(DateTime)),
                ("anniversary", POINTER(DateTime))]
    
    #-------------- enums -----------------
OK = 0
INV_FILE = 1
INV_CARD = 2
INV_PROP = 3
INV_DT = 4
WRITE_ERROR = 5
OTHER_ERROR = 6
# ---------------- C Structs END-------------------------


class noDBModel():
    def __init__(self):
        self.gotDB = False
    def add(self, contact):
        print("Database not connected")
    def get_summary(self):
        print("Database not connected")
        return []
    def get_contact(self,contact_id):
        print("Database not connected")
        return ()
    def get_current_contact(self):
        print("Database not connected")
        return {}
    def update_current_contact(self, details):
        print("Database not connected")
    def delete_contact(self, contact_id):
        print("Database not connected")
        
class ContactModel():
    def __init__(self, userName=None, pw=None, dbName=None):
    # Create a database in RAM.
        self.conn =  mysql.connector.connect(host="dursley.socs.uoguelph.ca",database=dbName,user=userName,password=pw)
        self.conn.autocommit = True
        self.cursor = self.conn.cursor()
        createQueryFile = "CREATE TABLE IF NOT EXISTS file ( \
            file_id INT AUTO_INCREMENT PRIMARY KEY, \
            file_name VARCHAR(60) NOT NULL,  \
            last_modified DATETIME, \
            creation_time DATETIME NOT NULL);"
            
        createQueryContact =  "CREATE TABLE IF NOT EXISTS contact ( \
            contact_id INT AUTO_INCREMENT PRIMARY KEY, \
            name VARCHAR(60) NOT NULL, \
            birthday DATETIME, \
            anniversary DATETIME, \
            file_id INT NOT NULL, \
            FOREIGN KEY (file_id) REFERENCES file(file_id) ON DELETE CASCADE);"
        self.cursor.execute(createQueryFile)
        self.cursor.execute(createQueryContact)
        self.online = True
        self.gotDB = True
        self.readCardsIntoDB()
        

        # Current contact when editing.
        self.current_id = None

    def add(self, contact):
        filename = contact["fileName"]
        self.cursor.execute('''
                            SELECT EXISTS(
                                SELECT 1 FROM file
                                WHERE file_name = %s);
                            ''', (filename,))
        fileExists = self.cursor.fetchone()[0]
        if (not(fileExists)):
            newName = create_string_buffer(contact["fn"].encode('utf-8'))
            if (self.createCardObj(filename, newName) != OK):
                return
            creationTime = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            self.cursor.execute('''
                INSERT INTO file(file_name, last_modified, creation_time)
                VALUES(%s, %s, %s);''', (filename, creationTime, creationTime))
            file_id = self.cursor.lastrowid
            self.cursor.execute('''
                                INSERT INTO contact(name, birthday, anniversary, file_id)
                                VALUES(%s, NULL, NULL, %s);
                                ''', (contact["fn"], file_id))

    def get_summary(self):
        self.cursor.execute(
            "SELECT file_name, file_id from file")
        return self.cursor.fetchall()

    def get_contact(self, contact_id):
        self.cursor.execute(
            "SELECT * from contact WHERE file_id=%s;", (str(contact_id),))
        result = self.cursor.fetchone()
        self.cursor.execute('''
                            SELECT file_name from file WHERE file_id=%s;''', (str(contact_id),))
        file_name = self.cursor.fetchone()[0]
        vcParserLib.dateToString.restype = c_char_p
        cardObj = self.cardsDict.get(file_name)
        if (cardObj == None):
            return {"fileName":"vCARD NOT FOUND", "fn":"N/A", "birthday":"N/A","anniversary":"N/A", 
                "otherProperties":"N/A"}
        bdayString = vcParserLib.dateToString(cardObj.contents.birthday).decode('utf-8')
        anniString = vcParserLib.dateToString(cardObj.contents.anniversary).decode('utf-8')
        numProp = str(vcParserLib.valuesLength(cardObj.contents.optionalProperties))
        return {"fileName":file_name, "fn":str(result[1]), "birthday":bdayString,"anniversary":anniString, 
                "otherProperties":numProp}

    def get_current_contact(self):
        if self.current_id is None:
            return {"fileName": "", "fn": "", "birthday": "", "anniversary": "", "otherProperties": ""}
        else:
            return self.get_contact(self.current_id)

    def update_current_contact(self, details):
        fail = False
        if self.current_id is None:
            self.add(details)
        elif (details["otherProperties"] == "N/A"):
            return
        else:
            nameToC = details["fileName"].lower().encode('utf-8')
            if (not(vcParserLib.validFileExt(nameToC))):
                return
            self.cursor.execute('''
                                SELECT file_name from file
                                WHERE file_id = %s;
                                ''', (str(self.current_id),))
            prevFileName = self.cursor.fetchone()[0]
            if (not(os.path.exists(cardsDir + prevFileName))):
                return
            cardObj = self.cardsDict.get(prevFileName)
            if (cardObj == None):
                return
            if (prevFileName != details["fileName"]):
                prevFilePath = cardsDir + prevFileName
                newName = cardsDir + details["fileName"]
                try:
                    if (not(os.path.exists(newName))):
                        os.rename(prevFilePath, newName)
                    else:
                        fail= True
                except Exception as e:
                    print("Error Changing File Name")
                    return
            newName = details["fn"].encode('utf-8')
            errCode = vcParserLib.editFN(newName, cardObj)
            
            if (errCode == OK and not(fail)):
                lastModTime = datetime.fromtimestamp(os.path.getmtime(cardsDir + details["fileName"])).strftime("%Y-%m-%d %H:%M:%S")
                self.cursor.execute('''
                                    UPDATE contact SET name = %s
                                    WHERE file_id = %s;
                                    ''', (details["fn"], str(self.current_id)))
                self.cursor.execute('''
                                UPDATE file SET file_name = %s, last_modified = %s
                                WHERE file_id = %s;
                                ''', (details["fileName"], lastModTime, str(self.current_id)))
                filePath = (cardsDir + details["fileName"]).encode('utf-8')
                errCode = vcParserLib.writeCard(filePath, cardObj)
                self.cardsDict.pop(prevFileName, None)
                self.cardsDict[details["fileName"]] = cardObj

    def delete_contact(self, contact_id):
        file_id = contact_id[1]
        self.cursor.execute('''
                            DELETE FROM file WHERE file_id = %s;''', (file_id,))
        self.cursor.execute('''
            DELETE FROM contact WHERE file_id=%s''', (file_id,))
    
    def getFileID(self, fileName):
        self.cursor.execute('''
                            SELECT EXISTS(
                                SELECT 1 FROM file
                                WHERE file_name = %s
                            );
                            ''', (fileName,))
        fileExists = self.cursor.fetchone()[0]
        if fileExists:
            self.cursor.execute('''
                                SELECT file_id FROM file
                                WHERE file_name = %s;
                                ''', (fileName,))
            return self.cursor.fetchone()[0]
        return None
    
    def getContactID(self, contact_name):
        self.cursor.execute('''
                            SELECT EXISTS(
                                SELECT 1 FROM contact
                                WHERE name = %s
                            );
                            ''', (contact_name,))
        contactExists = self.cursor.fetchone()[0]
        if contactExists:
            self.cursor.execute('''
                                SELECT contact_id FROM contact
                                WHERE name = %s
                                ''', (contact_name,))
            return self.cursor.fetchone()[0]
        return None
        
    def readCardsIntoDB(self):
        self.cardsDict = {}
        for file in filesList:
            self.cursor.execute('''
                                SELECT EXISTS(
                                    SELECT 1 FROM file
                                    WHERE file_name = %s
                                );
                                ''', (file,))
            fileExists = self.cursor.fetchone()[0]
            if (fileExists):
                card = POINTER(vCard)()
                filePath = cardsDir + file
                errorCode = vcParserLib.createValidate(filePath.encode('utf-8'), byref(card))
                self.cardsDict[file] = card
            elif (not(fileExists)):
                card = POINTER(vCard)()
                filePath = cardsDir + file
                errorCode = vcParserLib.createValidate(filePath.encode('utf-8'), byref(card))
                self.cardsDict[file] = card
                if (errorCode == OK):
                    fileName = file
                    fnValue = cast(vcParserLib.getFromFront(card.contents.fn.contents.values), c_char_p).value.decode('utf-8')
                    creationTime = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    lastModTime = datetime.fromtimestamp(os.path.getmtime(cardsDir + file)).strftime("%Y-%m-%d %H:%M:%S")
                    bdayValue = None
                    anniValue = None
                    anniValid = False
                    bdayValid = False
                    if (card.contents.birthday):
                        bdayValid = vcParserLib.hasDT(card.contents.birthday)
                    if (bdayValid and not(card.contents.birthday.contents.isText)):
                        bdayDate = datetime.strptime(card.contents.birthday.contents.date.decode('utf-8'), "%Y%m%d").strftime("%Y-%m-%d")
                        bdayTime = datetime.strptime(card.contents.birthday.contents.time.decode('utf-8'), "%H%M%S").strftime("%H:%M:%S")
                        bdayValue = bdayDate + " " + bdayTime
                    if (card.contents.anniversary):
                        anniValid = vcParserLib.hasDT(card.contents.anniversary)
                    if (anniValid and not(card.contents.anniversary.contents.isText)):
                        anniDate = datetime.strptime(card.contents.anniversary.contents.date.decode('utf-8'), "%Y%m%d").strftime("%Y-%m-%d")
                        anniTime = datetime.strptime(card.contents.anniversary.contents.time.decode('utf-8'), "%H%M%S").strftime("%H:%M:%S")
                        anniValue = anniDate + " " + anniTime
                    self.cursor.execute('''
                                        INSERT INTO file(file_name, last_modified, creation_time)
                                        VALUES (%s, %s, %s);
                                        ''', (fileName, lastModTime, creationTime))
                    fileID = self.cursor.lastrowid
                    self.cursor.execute('''
                                        INSERT INTO contact(name, birthday, anniversary, file_id)
                                        VALUES(%s, %s, %s, %s);
                                        ''', (fnValue, bdayValue, anniValue, fileID))
    def createCardObj(self, fileName, fn):
        if (os.path.exists(cardsDir + fileName)):
            return INV_CARD
        card = POINTER(vCard)()
        filePath = (cardsDir + fileName).encode('utf-8')
        errorCode = vcParserLib.initCard(fn, filePath, byref(card))
        if (errorCode == OK):
            self.cardsDict[fileName] = card
        return errorCode
    
    def queryJuneBdays(self):
        self.cursor.execute('''
                            SELECT name, birthday, DATEDIFF(NOW(), birthday) DIV 365 , contact_id  FROM contact
                            WHERE MONTH(birthday) = 6
                            ORDER BY DATEDIFF(NOW(), birthday) DESC;
                            ''')
        result = self.cursor.fetchall()
        formattedResult = []
        for contact in result:
            spacing = " " * (24 - len(contact[0]))
            bday = spacing + contact[1].strftime("%B %d, %Y") + "   Age: " + str(contact[2])
            name = "             " + contact[0]
            tempTuple = (name + bday, contact[3])
            formattedResult.append(tempTuple)
        return formattedResult
    
    def queryAll(self):
        self.cursor.execute('''
                            SELECT c.contact_id, c.name, c.birthday, c.anniversary, f.file_name
                            FROM contact AS c, file AS f
                            WHERE c.file_id = f.file_id
                            ORDER BY f.file_name;
                            ''')
        result = self.cursor.fetchall()
        formattedResult = []
        x = 0
        for contact in result:
            bday = "                  |"
            anni = "                  |"
            if contact[2]:
                bday = contact[2].strftime("%x %X")
                bdayLen = len(bday)
                bday = bday + " " * (18 - bdayLen) + "|"
            if contact[3]:
                anni = contact[3].strftime("%x %X")
                anniLen = len(anni)
                anni = anni + " " * (18 - anniLen) + "|"
            id = str(contact[0])
            if (contact[0] >= 100): 
                id = id + "  |        "
            elif (contact[0] >= 10): 
                id = id + "   |        "
            else:
                id = id + "    |        "
            name = contact[1] + " " * (abs(22 - len(contact[1]))) + "|"
            tempTuple = (id + name + bday + anni  + contact[4], str(x))
            x += 1
            formattedResult.append(tempTuple)
        return formattedResult
            
        
            

class ListView(Frame):
    def __init__(self, screen, model):
        super(ListView, self).__init__(screen,
                                       screen.height * 2 // 3,
                                       screen.width * 2 // 3,
                                       on_load=self._reload_list,
                                       hover_focus=True,
                                       can_scroll=False,
                                       title="vCards List",
                                       has_border=True,
                                       name=None,
                                       is_modal=False)
        # Save off the model that accesses the contacts database.
        self._model = model
        self.set_theme("bright")
        
        
        # Create the form for displaying the list of contacts.
        self._list_view = ListBox(
            Widget.FILL_FRAME,
            model.get_summary(),
            name="vCards",
            add_scroll_bar=True,
            on_change=self._on_pick,
            on_select=self._edit)
        self._edit_button = Button("Edit", self._edit)
        self._query_button = Button("DB Queries", self._db_query)
        # self._delete_button = Button("Delete", self._delete)
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        layout.add_widget(self._list_view)
        layout.add_widget(Divider())
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Add", self._add), 0)
        layout2.add_widget(self._edit_button, 1)
        layout2.add_widget(self._query_button, 2)
        layout2.add_widget(Button("Quit", self._quit), 3)
        self.fix()
        self._on_pick()
        self._screen = screen
    
    def changeModel(self, newModel):
        self._model = newModel
    def _on_pick(self):
        self._edit_button.disabled = self._list_view.value is None
        if (not(self._model.gotDB)):
            self._query_button.disabled = True
        else:
            self._query_button.disabled = False

    def _reload_list(self, new_value=None):
        self._list_view.options = self._model.get_summary()
        self._list_view.value = new_value

    def _add(self):
        self._reload_list()
        self._model.current_id = None
        raise NextScene("Edit Contact")

    def _edit(self):
        self.save()
        self._model.current_id = self.data["vCards"]
        raise NextScene("Edit Contact")

    def _delete(self):
        self.save()
        self._model.delete_contact(self.data["vCards"])
        self._reload_list()
        
    def _db_query(self):
        raise NextScene("Query DB")
    
        
    def _quit(self):
        if (self._model.gotDB):
            # vcParserLib.deleteCard.argtypes = [POINTER(vCard)]
            # vcParserLib.deleteCard.restype = None
            # for key in self._model.cardsDict.keys():
            #     card = self._model.cardsDict[key]
            #     vcParserLib.deleteCard(card)
            # self._model.cursor.close()
            self._model.conn.close()
        raise StopApplication("User pressed quit")


class ContactView(Frame):
    def __init__(self, screen, model):
        super(ContactView, self).__init__(screen,
                                          screen.height * 2 // 3,
                                          screen.width * 2 // 3,
                                          hover_focus=True,
                                          can_scroll=False,
                                          title="vCard Details",
                                          reduce_cpu=True)
        # Save off the model that accesses the contacts database.
        self._model = model
        self.set_theme("bright")
        # Create the form for displaying the list of contacts.
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        self.filename = Text(label="File name:", name="fileName")
        layout.add_widget(self.filename)
        self.contact = Text("Contact:", "fn")
        layout.add_widget(self.contact)
        layout.add_widget(Text("Birthday:", "birthday", readonly=True))
        layout.add_widget(Text("Anniversary:", "anniversary", readonly=True))
        layout.add_widget(Text("Other Properties:", "otherProperties", readonly=True))
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 3)
        self.fix()
    
    def changeModel(self, newModel):
        self._model = newModel
    def reset(self):
        # Do standard reset to clear out form, then populate with new data.
        super(ContactView, self).reset()
        self.data = self._model.get_current_contact()

    def _ok(self):
        if (not (self._model.gotDB)):
            print("No DB connected")
            return
        self.save()
        if (self.data["otherProperties"] == "N/A"):
            return print("Invalid File")
        nameToC = self.data["fileName"].lower().encode('utf-8')
        if (not(vcParserLib.validFileExt(nameToC))):
            print("Invalid File Extension")
        else:
            self._model.update_current_contact(self.data)
            raise NextScene("Main")

    @staticmethod
    def _cancel():
        raise NextScene("Main")
    
    
class DBView(Frame):
    def __init__(self, screen, model):
        super(DBView, self).__init__(screen,
                                       screen.height * 2 // 3,
                                       screen.width * 2 // 3,
                                       hover_focus=True,
                                       can_scroll=False,
                                       on_load=self._reload_list,
                                       title="DB Queries",
                                       has_border=True,
                                       name=None,
                                       is_modal=False)
        # Save off the model that accesses the contacts database.
        self._model = model
        self.set_theme("bright")
        
        
        # Create the form for displaying the list of contacts.
        self._list_view = ListBox(
            Widget.FILL_FRAME,
            options=[],
            name="Queries",
            add_scroll_bar=True)
        # self._delete_button = Button("Delete", self._delete)
        self.heading = Layout([1, 1, 1, 1, 1])
        self.add_layout(self.heading)
        self.heading.add_widget(Label("c_id |"), 0)
        self.heading.add_widget(Label("Name             |"), 1)
        self.heading.add_widget(Label("Birthday         |"), 2)
        self.heading.add_widget(Label("Anniversary      |"), 3)
        self.heading.add_widget(Label("file_name        "), 4)
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        layout.add_widget(self._list_view)
        layout.add_widget(Divider())
        layout2 = Layout([1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Display all contacts", self._show_all), 0)
        layout2.add_widget(Button("Find contacts born in June", self._june_bday), 1)
        layout2.add_widget(Button("Cancel", self._cancel), 2)
        self.fix()
        self._screen = screen
    
    def changeModel(self, newModel):
        self._model = newModel

    def _reload_list(self, new_value=None):
        self._list_view.value = new_value

    def _show_all(self):
        self._list_view.options = self._model.queryAll()
        
    def _june_bday(self):
        self._list_view.options = self._model.queryJuneBdays()
    
        
    @staticmethod
    def _cancel():
        raise NextScene("Main")
    
class LoginView (Frame):
    def __init__(self, screen, mainView, cView, dbView):
        super(LoginView, self).__init__(screen,
                                          screen.height * 2 // 3,
                                          screen.width * 2 // 3,
                                          hover_focus=True,
                                          can_scroll=False,
                                          title="Login",
                                          reduce_cpu=True)
     # Save off the model that accesses the contacts database.
        self.set_theme("bright")
        # Create the form for displaying the list of contacts.
        layout = Layout([100], fill_frame=True)
        self._mainView = mainView
        self._cView = cView
        self._dbView = dbView
        self.add_layout(layout)
        self.userName = Text(label="Username:", name="Username", max_length = 25)
        layout.add_widget(self.userName)
        self.password = Text(label="Password:", name="Password", max_length =25)
        layout.add_widget(self.password)
        self.db_name = Text(label="DB Name:", name="db_name", max_length = 25)
        layout.add_widget(self.db_name)
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Login", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 3)
        self.fix()
    
    def _ok(self):
        self.save()
        userName = self.data["Username"].lower()
        pw =self.data["Password"]
        dbName = self.data["db_name"].lower()
        try:
            global contacts
            contacts = ContactModel(userName, pw, dbName)
            self._mainView.effects[0].changeModel(contacts)
            self._cView.effects[0].changeModel(contacts)
            self._dbView.effects[0].changeModel(contacts)
            
            raise NextScene("Main")
        except mysql.connector.Error as err:
            print("Something went wrong: {}".format(err))        

    def _cancel(self):
        raise NextScene("Main")



def demo(screen, scene):
    mainView = Scene([ListView(screen, contacts)], -1, name="Main")
    cView = Scene([ContactView(screen, contacts)], -1, name="Edit Contact")
    dbView = Scene([DBView(screen, contacts)], -1, name = "Query DB")
    scenes = [
        Scene([LoginView(screen, mainView, cView, dbView)], -1, name="Login"),
        mainView,
        cView,
        dbView
    ]

    screen.play(scenes, stop_on_resize=True, start_scene=scene, allow_int=True)


contacts = noDBModel()
last_scene = None
while True:
    try:
        Screen.wrapper(demo, catch_interrupt=True, arguments=[last_scene])
        sys.exit(0)
    except ResizeScreenError as e:
        last_scene = e.scene