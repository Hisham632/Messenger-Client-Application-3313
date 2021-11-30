# import the required modules
from socket import AF_INET, socket, SOCK_STREAM
from threading import Thread
import tkinter as tk

# method to recieve messages from the thread
def receive():
    while True:
        try:
            message = client_socket.recv(BUFFER_SIZE).decode("utf8")
            message_list.insert(tk.END, message)
            message_list.see(tk.END)
        except OSError:
            break

# send message to the socket
def send(event=None):
    message = client_msg.get()

    client_msg.set("")      # empty the message field
    global current_room

    if message == "{quit}":
        client_socket.send(bytes(client_username.get() + " left the room", "utf8"))
        client_socket.close()
        window.quit()
        return

    client_socket.send(bytes(client_username.get() + ": " + message, "utf8"))

# send exit message to the server
def on_exit(event=None):
    # convey exit message
    client_msg.set("{quit}")
    send()

# method to notify the server of change in rooms
def switch_rooms():
    global current_room
    current_room = ((chatRoomSelected.get()).split(' '))[2]
    client_socket.send(bytes("/" + current_room, "utf8"))
    message_list.delete(0, tk.END)
    message_list.insert(tk.END, "Current room: " + str(current_room))
    message_list.see(tk.END)

# global variables to keep track of the total chat rooms and the current room the client is using
room_count = 0
current_room = 0

# create the GUI
window = tk.Tk()
window.title("Swag Rooms")

messages_frame = tk.Frame(window)
client_msg = tk.StringVar()  # Message field variable
client_msg.set("")

client_username = tk.StringVar() # username field variable
client_username.set("")

scrollbar = tk.Scrollbar(messages_frame)  # So that the message field ca see previous messages

# Text box displaying the messages
message_list = tk.Listbox(messages_frame, height=30, width=100, yscrollcommand=scrollbar.set)
scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
message_list.pack(side=tk.LEFT, fill=tk.BOTH)
message_list.pack()
messages_frame.pack()

# label and input field for the username field
username_label = tk.Label(window, text="Your username: ")
username_label.pack()
username_field = tk.Entry(window, textvariable=client_username)
username_field.pack()

# label and input field for the message field
message_label = tk.Label(window, text="Enter message: ")
message_label.pack()
entry_field = tk.Entry(window, textvariable=client_msg, width=50)
entry_field.bind("<Return>", send)
entry_field.pack()
send_button = tk.Button(window, text="Send", command=send)
send_button.pack()

# call the on_exit method on change
window.protocol("WM_DELETE_WINDOW", on_exit)

# configurations to connect to the server
HOST = "localhost"
PORT = 3005
BUFFER_SIZE = 1024
ADDRESS = (HOST, PORT)

# python socket
client_socket = socket(AF_INET, SOCK_STREAM)
client_socket.connect(ADDRESS)

# get the total number of chat rooms from the server
initial_msg = client_socket.recv(BUFFER_SIZE).decode("utf8")
room_count = int(initial_msg)
chatRoomSelected = tk.StringVar(window)
chatRoomSelected.set("Available Chat Rooms")

rooms_list = []

# add each of the rooms to the list of rooms
for i in range(room_count):
    rooms_list.append("Swag Room " + str(i + 1))

chat_rooms = tk.OptionMenu(window, chatRoomSelected, *rooms_list)
chat_rooms.pack()
switch_button = tk.Button(window, text="Switch Room", command=switch_rooms)
switch_button.pack()

receive_thread = Thread(target=receive)
receive_thread.start()
window.resizable(width=False, height=False)    # Disable window resizing
tk.mainloop() # display the GUI