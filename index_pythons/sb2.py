"""
The main user interface the user types commands into.

Sends POST requests that the teensy then listens for.
Displays POST requests.
"""

import _mysql
import cgi

with open('sb4.py', 'r') as myfile:
    body_html=myfile.read().replace("'''","").replace("#!/home/6S08/joerun.py","")

exec(open("/var/www/html/student_code/LIBS/s08libs.py").read())

site = 'allanc' #replace with your own kerberos (eventually)
users = {'iotlong':'damnanshula'}  #replace with your own user list (eventually)

# Do some HTML formatting at the very top!
print( "Content-type:text/html\r\n\r\n")
print('<html>')

method_type = get_method_type() #use this for figuring out if it is a GET or POST!
form = cgi.FieldStorage() #get specified parameters!

if method_type == "GET": #expecting a GET request.
    if 'username' in form.keys() and 'password' in form.keys():
        username = form['username'].value
        password = form['password'].value
        if users[username] != password:
            print('Sorry :( %s. Your password is not correct. Consult your administrator.' %(username))
        else:
            print(body_html %(site,username,site,username,str(users)))
    else:
        print("You need to specify a username and password as GET parameters")

print('</html>')
