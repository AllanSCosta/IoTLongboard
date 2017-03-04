import _mysql
import cgi
import datetime
exec(open("/var/www/html/student_code/LIBS/s08libs.py").read())
print( "Content-type:text/html\r\n\r\n")
#system specific variables:
site = 'allanc'# site  name (your kerberos)
users = {'iotlong':'damnanshula'}  #replace with your own user list (eventually)
method_type = get_method_type()
form = cgi.FieldStorage() #get specified parameters!
if method_type == 'POST':
    #you need to define the variables sender, recipient, message for use in the query creation below
    #connect to database:
    cnx = _mysql.connect(user='student', passwd='6s08student',db='iesc')
    #create a mySQL query and commit to database relevant information for logging message
    sender = form['sender'].value
    recipient = "iotlong"
    message = form['message'].value
    if len(message) != 0:
        query = ("INSERT INTO messenger (sender, recipient, message, site) VALUES ('%s','%s','%s','%s')" %(sender,recipient,message,site)) #logs the message
        cnx.query(query)
        cnx.commit()
elif method_type == 'GET':
    #you need to define the variable user for use in the query creation below
    #connect to database
    username = form['recipient'].value
    user = username
    cnx = _mysql.connect(user='student', passwd='6s08student',db='iesc')
    #Create query to database to get all messages meeting certain criteria (to a user and associated with the site
    query = ("SELECT * FROM messenger WHERE recipient='%s' AND site='%s' AND timestamp >= timestamp(date_sub(now(), interval 5 minute))" %(user,site))
    cnx.query(query)
    result =cnx.store_result()
    rows = result.fetch_row(maxrows=0,how=0)
    messages = []
    processed_msg = ""
    for row in rows:
        messages.append([e.decode('utf-8') if type(e) is bytes else e for e in row])
    for i in range(len(messages)):
        if i < len(messages)-1:
            processed_msg += "<p>> %s</p>" % (messages[i][2])
        else:
            processed_msg += "<p>>> %s</p>" % (messages[i][2])
        if messages[i][2] == "help":
            processed_msg += "<p> The IoT Longboard can be activated by long(ON), deactivated by long(OFF); To estimate the time to get to a given place, use function time(-name of the place-). To estimate distance, dist(-name of the place-). In order to activate GPS coordinates and request light-based orientation to a place, type orient(-name of the place-). The IoT Longboard will store information, so you won't need to worry about data in your way. When you arrive, IoT Longboard will automatically reset its stored maps in order for you to safely get back home.</p>"
        elif messages[i][2] == "easteregg":
            processed_msg += "<p> HUEHUE </p>"
    print("<html>"+processed_msg+"</html>")
