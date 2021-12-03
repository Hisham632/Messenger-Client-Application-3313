package com.company;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.plaf.metal.DefaultMetalTheme;
import javax.swing.plaf.metal.MetalLookAndFeel;
import javax.swing.plaf.metal.OceanTheme;
import java.awt.*;
import java.awt.event.*;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ConnectException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.*;
import java.net.UnknownHostException;
import java.util.Scanner;


public class FrontEndMain extends JFrame
{
    private JLabel labelName;
    private JTextField textName;
    private JLabel messageName;
    private JButton buttonConnection;
    private JButton buttonDisconnection;
    private JTextArea textArea;
    private JScrollPane scrollPane;
    private JTextField textField;
    private JButton buttonSend;
    private JComboBox<String> comboBox;
    private JLabel labelRoom;
    private Socket socket;
    private InputStream socketIn;
    private OutputStream socketOut;
    private boolean connected;
    private boolean initialConnection;
    private PrintWriter pw;
    private JButton styleButton;
    private JLabel styleLabel;

    public static void main(String[] args)
    {
        new FrontEndMain();
    }

    public FrontEndMain()
    {

        JFrame.setDefaultLookAndFeelDecorated(true);
        this.connected = false;
        this.initialConnection = true;
        this.setSize(950, 600);
        this.setTitle("ChatRoom");

        final String[] chatRooms = {"Please Connect First"};
        comboBox = new JComboBox<>(chatRooms);


        JPanel panelTop = new JPanel();
        panelTop.setLayout(new FlowLayout());
        labelName = new JLabel("Name: ");
        panelTop.add(labelName);
        textName = new JTextField("", 25);
        panelTop.add(textName);
        labelRoom = new JLabel("Select Chat Room: ");
        panelTop.add(labelRoom);
        panelTop.add(comboBox);

        textArea = new JTextArea();
        textArea.setLineWrap(true);
        textArea.setWrapStyleWord(true);
        scrollPane = new JScrollPane(textArea);
        textArea.getDocument().addDocumentListener(new DocumentListener() {

            @Override
            public void removeUpdate(DocumentEvent arg0) {
                // TODO Auto-generated method stub

            }

            @Override
            public void insertUpdate(DocumentEvent arg0) {
                textArea.setCaretPosition(textArea.getText().length());

            }

            @Override
            public void changedUpdate(DocumentEvent arg0) {
                // TODO Auto-generated method stub

            }
        });

        JPanel panelBottom = new JPanel();
        panelBottom.setLayout(new FlowLayout());
        messageName = new JLabel("Message:");
        panelBottom.add(messageName);
        textField = new JTextField(30);
        panelBottom.add(textField);
        buttonSend = new JButton("Send");
        SendMessage sendAction = new SendMessage();
        buttonSend.addActionListener(sendAction);
        buttonSend.setEnabled(false);
        panelBottom.add(buttonSend);
        buttonConnection = new JButton("Connect");
        Connection connectionAction = new Connection(textArea);
        buttonConnection.addActionListener(connectionAction);
        panelBottom.add(buttonConnection);
        buttonDisconnection = new JButton("Disconnect");
        Disconnection disconnectAction = new Disconnection();
        buttonDisconnection.addActionListener(disconnectAction);
        buttonDisconnection.setEnabled(false);
        panelBottom.add(buttonDisconnection);

        comboBox.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                if(!initialConnection) {
                    System.out.println("I am changing rooms");
                    String numb = String.valueOf(comboBox.getSelectedItem());
                    int number;
                    String mynum = numb.split(" ")[2];
                    number = Integer.parseInt(mynum);
                    String sendStr = "/" + number + "\r";

                    try {
                        socketOut.write(sendStr.getBytes());
                    } catch (IOException E) {
                        System.out.println(E.getMessage());
                    }

                    textArea.setText("Current Room: " + mynum + "\n");
                }
            }
        });

        textField.addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent ke){
                if(ke.getKeyChar() == KeyEvent.VK_ENTER){
                    buttonSend.doClick();
                }
            }
        });

        this.setLayout(new BorderLayout(5,5));
        this.add(panelTop, BorderLayout.NORTH);
        this.add(scrollPane, BorderLayout.CENTER);
        this.add(panelBottom, BorderLayout.SOUTH);


        this.addWindowListener(new WindowAdapter()
        {
            public void windowClosing(WindowEvent e)
            {

                if(connected)
                {
                    try
                    {
                        if(connected)
                        {
                            socketOut.write("{quit}\r".getBytes());
                            socket.close();
                        }
                    }
                    catch (IOException e1)
                    {
                        System.out.println("Socket is closed");
                    }
                }
                System.out.println("The client program has been closed");
                System.exit(0);
            }
        });

        this.setVisible(true);
    }

    class ClientReceive extends Thread
    {
        private int roomNumber;

        public ClientReceive(int num)
        {
            this.roomNumber = num;
        }

        public void run()
        {
            try
            {
                BufferedReader br = new BufferedReader(new InputStreamReader(socket.getInputStream()));

                while (true)
                {

                    if(socket.isClosed())
                    {
                        System.out.println("The client thread has been closed");
                        break;
                    }
                    if(initialConnection)
                    {
                        int numRooms = Integer.parseInt(br.readLine());
                        comboBox.removeAllItems();
                        for (int i = 1; i <= numRooms; i++) {
                            String strroom = "Swag Room " + i;
                            comboBox.addItem(strroom);
                        }
                        System.out.println("done");

                        initialConnection = false;
                        continue;
                    }

                    System.out.println(br.readLine());
                    // send the message
                    String text = textArea.getText() + "\n" + br.readLine();
                    textArea.setText(text);
                }
            }
            catch (IOException e)
            {
                System.out.println("Goodbye!!");
            }
        }
    }

    class SendMessage implements ActionListener
    {
        public void actionPerformed(ActionEvent e)
        {
            if(connected)
            {
                try
                {
                    String sendText = textField.getText();

                    // get the message
                    String responseText = "\r" + textName.getText() + ": " + sendText + "\r";

                    // send message to the socket
                    socketOut.write(responseText.getBytes());

                    // empty the message input box
                    textField.setText("");
                }
                catch(Exception er)
                {
                    System.out.println(er.getMessage());
                }
            }
            else
            {
                System.out.println("Cannot send message, I am not connected");
            }
        }

    }

    class Connection implements ActionListener
    {
        JTextArea ta;

        Connection(JTextArea ta)
        {
            this.ta = ta;
        }

        public void actionPerformed(ActionEvent e)
        {
            try
            {
                //clear the current text within the JTextArea
                this.ta.setText("");

                String numb = String.valueOf(comboBox.getSelectedItem());
                int number;
                String mynum = numb.split(" ")[2];

                if(initialConnection)
                {
                    System.out.println("In");
                    number = 1;
                }
                else
                {
                    number = Integer.parseInt(mynum);
                }

                socket = new Socket("172.30.141.181", 3005);
                socketIn = socket.getInputStream();
                socketOut = socket.getOutputStream();

                if(socket.isConnected())
                {
                    System.out.println("successfully connected");
                    connected = true;
                    buttonConnection.setEnabled(false);
                    buttonDisconnection.setEnabled(true);
                    buttonSend.setEnabled(true);
                    new ClientReceive(number).start();
                }
            }
            catch (UnknownHostException e1)
            {
                System.out.println("Server was not found");
            }
            catch (ConnectException e2)
            {
                System.out.println("Unable to connect to the Server");
            } catch (IOException E) {
                System.out.println(E.getMessage());
            }
        }

    }

    class Disconnection implements ActionListener
    {

        public void actionPerformed(ActionEvent e)
        {
            if (connected)
            {
                try
                {
                    socketOut.write("{quit}\r".getBytes());
                    socket.close();
                    if (socket.isClosed())
                    {
                        connected = false;
                        buttonConnection.setEnabled(true);
                        buttonDisconnection.setEnabled(false);
                    }
                }
                catch (IOException e1)
                {
                    System.out.println("Server was not running... shutting down client socket now");
                }
            }
        }
    }
}
