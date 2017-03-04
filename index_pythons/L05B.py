import _mysql
import cgi
import datetime

exec(open("/var/www/html/student_code/LIBS/s08libs.py").read())
print( "Content-type:text/html\r\n\r\n")


#system specific variables:
site = 'anshula'# site  name (your kerberos)
users = {'person1':'1', 'person2':'2'} 

method_type = get_method_type()
form = cgi.FieldStorage() #get specified parameters!

if method_type == 'POST':

    recipient = form['recipient'].value
    message = form['message'].value

    #you need to define the variables recipient, message for use in the query creation below
    #connect to database:
    cnx = _mysql.connect(user='student', passwd='6s08student',db='iesc') 

    #create a mySQL query and commit to database relevant information for logging message
    query = ("INSERT INTO messenger (sender, recipient, message, site) VALUES ('', '%s','%s','%s')" %(recipient,message,site)) #logs the message
    cnx.query(query)
    cnx.commit()

elif method_type == 'GET':

    username = form['recipient'].value
    platform = form['platform'].value # "teensy" or "browser"

    #you need to define the variable user for use in the query creation below
    #connect to database
    user =username
    cnx = _mysql.connect(user='student', passwd='6s08student',db='iesc') 
    #Create query to database to get all messages meeting certain criteria (to a user and associated with the site
    if platform == "teensy":
        query = ("SELECT * FROM messenger WHERE recipient='%s' AND site='%s' AND timestamp >= timestamp(date_sub(now(), interval 1 minute))" %(user,site))
    elif platform == "browser": 
        query = ("SELECT * FROM messenger WHERE recipient='%s' AND site='%s' AND timestamp >= timestamp(date_sub(now(), interval 5 minute))" %(user,site))
    cnx.query(query)
    result =cnx.store_result()  
    rows = result.fetch_row(maxrows=0,how=0) 
    messages = []
    #generate list of messages (and take care of unicode issues so everything is a Python String)
    for row in rows:
        messages.append([e.decode('utf-8') if type(e) is bytes else e for e in row])

    # format the messages array so it prints better
    print(type(messages))

    formatted_messages = "<html>"
    if platform == "teensy":
        for bogus, recipient, message, date, site in messages: 
            formatted_messages += message + "\n"
    elif platform == "browser":   
        bogus, recipient, message, date, site = messages[-1] # take only the last message 
        formatted_messages += "<i>(" + date + ")</i>: " + message + "<br />"
    formatted_messages += "</html>"

    # print(messages)
    print(formatted_messages)
