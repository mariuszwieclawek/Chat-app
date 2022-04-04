# Chat-app
The theme of the project is a client-server text messenger that will be
used the TCP and UDP protocols with the use of multicast addresses. Connection between the server,
and the clients are established using the TCP protocol, while the UDP protocol is used for
ensuring communication between clients using a multicast address.
The server stores the login and MAC address of registered users in a text file that
acts as a user base.

# Documentation
documentation: [documentation.pdf](https://github.com/mariuszwieclawek/Chat-app/files/8410460/documentation.pdf)

# How it works:
![image](https://user-images.githubusercontent.com/57256517/161555181-e99ef6f3-856d-4a22-9155-2079b95cb2f2.png)
On the server side, we can see that it has displayed information about the client joining.
The console displays IP and MAC addresses as well as a message informing about registration / logging in
user. When analyzing the client terminal, we see an example of an exchange of views between users.
The situation of logging in (station with the address 192.168.56.32) and registration (station at
address 192.168.56.34). It can also be noticed that additionally the server sends information to the connected
already customers, about the arrival of a new user
