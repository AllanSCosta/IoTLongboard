"""
Server-side python that the teensy submits a get request to (with GPS coords and end location)
Prints out vertices of path
"""

import cgi
import urllib.request
import json
import math

# http://iesc-s2.mit.edu/student_code/anshula/lab05/L05B.py

import urllib.request
import json

def get_json(url):
        response = urllib.request.urlopen(url)
        content = response.read().decode('ascii', errors='ignore')
        return json.loads(content)


def get_vertices(origin_lat, origin_lon, destination, api_key):
    url = "https://maps.googleapis.com/maps/api/directions/json?key=%s&mode=bicycling&origin=%f,%f&destination=%s" % (api_key, origin_lat, origin_lon, destination)
    print(url)
    print("</br></br>")

    # get the first route
    route = get_json(url)["routes"][0]

    # get the first leg (whatever that is)
    leg = route["legs"][0]

    duration = leg["duration"]["text"]
    print("This trip will take " + duration)
    print("</br></br>")

    end_address = leg["end_address"]
    print("Your end address is " + end_address)
    print("</br></br>")

    steps = leg["steps"]
    vertices=[(step["end_location"]["lat"],step["end_location"]["lng"]) for step in steps]

    print("The vertices of the trip is <vertices>" + str(vertices) + "</vertices>")

    print("The number of vertices on this trip is <num_vertices>" + str(len(vertices))+ "</numvertices>")

def get():

    # Do some HTML formatting at the very top!
    print( "Content-type:text/html\r\n\r\n")
    print('<html>')
    print('<h1>Display Directions</h1>')
    print('<h4>Given a start location and end location place id in a GET parameter, this page will display vertices of directions to your location!</h4>')

    api_key="AIzaSyCfZnuu99D4Siyw4aveTAGqro6fampwnpM"

    # --------------------------------------------------------------------------------------- #
    # Get default start and end location
    # --------------------------------------------------------------------------------------- #

    # start location is killian court
    origin_lat = 42.359163
    origin_lon = -71.091826

    # end location is Ashdown
    destination = "place_id:ChIJFdveNv5544kRNZfvlMVKF4M" 

    # --------------------------------------------------------------------------------------- #
    # Get start and end location from get request
    # --------------------------------------------------------------------------------------- #

    form = cgi.FieldStorage() #get specified parameters

    if 'origin_lat' in form.keys(): #form is a dictionary which has the get parameters
        origin_lat = float(form['origin_lat'].value) 
    if 'origin_lon' in form.keys():
        origin_lon = float(form['origin_lon'].value) 
    if 'destination' in form.keys():
        destination=form['destination'].value 
    
    # --------------------------------------------------------------------------------------- #
    # Send request to Google Maps API
    # --------------------------------------------------------------------------------------- #


    try:
        get_vertices(origin_lat, origin_lon, destination, api_key)

    except Exception as e:
        print(e)
        print("Fail")

    print('</html>')

get()

