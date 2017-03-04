'''
<heading>

    <script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.4.1/jquery.min.js"></script>
    <link rel="stylesheet" href="https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquerymobile/1.4.5/jquery.mobile.min.js"></script>

    <script type="text/javascript">
        $(document).ready(function() {
            clearIt = function(){
                console.log("sending message");
                console.log($("#message").val());
                console.log($("#recipient").val());
                $.ajax({
                    url: 'http://iesc-s2.mit.edu/student_code/%s/lab07/message.py/',
                    method:"POST",
                    type:"POST",
                    data: "recipient="+$('#recipient option:selected').text()+"&message="+$("#message").val()+"&sender=%s",
                    success: function(data){
                        $("#message").val("");
                        console.log(data);
                    },
                });
            };

            $("#send").click(function(){
                console.log("clicked");
                clearIt();
            });

            function periodicMessageUpdate(){
                console.log("requesting");

                // display lastCall commands
                $.ajax({
                    url: 'http://iesc-s2.mit.edu/student_code/%s/lab07/message.py',
                    data: "recipient=%s",
                    success: function(data){
                        console.log(data);
                        $("#display").html(decodeURI(data));
                    },
                    complete: function(){
                        setTimeout(periodicMessageUpdate,1000);
                    }
                });

                // display velocity update
                $.ajax({
                    url: 'http://iesc-s2.mit.edu/student_code/anshula/lab05/L05B.py',
                    data: "recipient=person1&platform=browser",
                    success: function(data){
                        console.log(data);
                        $("#statusupdate").html(decodeURI(data));
                    },
                    complete: function(){
                        //setTimeout(periodicMessageUpdate,1000);
                    }
                });
            };
            all_users = %s;
            $.each(all_users, function(key, value) {
             $('#recipient')
             .append($('<option>', { value : key })
              .text(key));
         });

            setTimeout(periodicMessageUpdate,1000);
        });
    </script>
</heading>
<style>

    /* Terminal stuff */

    body{
        background-color:#404040;
        font-family:Century Gothic;
    }
    #display {
        border-radius: 5px;
        border: 2px solid gray;
        padding: 20px;
        width: 450px;
        height: auto;
        text-align:left;
        background-color:black;
        color:white;
    }
    #terminal-container{
        position:relative;
        margin-right:auto;
        margin-left:auto;
        text-align:center;
        width:450px;
        color:gray;
    }
    #send{
        display:inline !important;
        color:white;
        cursor:pointer;
    }
    #message{
        background-color:#202020;
        color:white;
        width:420px;
        display:inline !important;
    }
    #desc{
        font-size:15px;
        color:gray;
    }

    /* Place ID finder stuff */

    #map {
        height: 300px;
        width: 600px;
        margin:50px auto;
    }
    .controls {
        background-color: #fff;
        border-radius: 2px;
        border: 1px solid transparent;
        box-shadow: 0 2px 6px rgba(0, 0, 0, 0.3);
        box-sizing: border-box;
        font-family: Roboto;
        font-size: 15px;
        font-weight: 300;
        height: 29px;
        margin-left: 17px;
        margin-top: 10px;
        outline: none;
        padding: 0 11px 0 13px;
        text-overflow: ellipsis;
        width: 400px;
    }

    .controls:focus {
        border-color: #4d90fe;
    }
</style>
<body>

    <!-- Terminal -->
    <div id="terminal-container">
        <h1>IoT LongBoard</h1>
        <p id="desc">Welcome to the IoT Longboard! Some of our comands include: light(ON/OFF), color(R,B,G), and help</p>

        <input type="text" id="message" value=""> <div id="send">>></div>
        <p></p>

        <div id="display">
        </div>

        <div id="statusupdate">
        </div>
    </div>

    <!-- Place ID Finder -->

    <input id="pac-input" class="controls" type="text"
    placeholder="Enter a location">
    <div id="map"></div>
    <script src="https://maps.googleapis.com/maps/api/js?key=AIzaSyCfZnuu99D4Siyw4aveTAGqro6fampwnpM&libraries=places&callback=initMap"
    async defer></script>
    <script>
      // this is the code from the google demo

      function initMap() {
        var map = new google.maps.Map(document.getElementById('map'), {
          center: {lat: 42.373616, lng: -71.109734},
          zoom: 13
      });

        var input = document.getElementById('pac-input');

        var autocomplete = new google.maps.places.Autocomplete(input);
        autocomplete.bindTo('bounds', map);

        map.controls[google.maps.ControlPosition.TOP_LEFT].push(input);

        var infowindow = new google.maps.InfoWindow();
        var marker = new google.maps.Marker({
          map: map
      });
        marker.addListener('click', function() {
          infowindow.open(map, marker);
      });

        autocomplete.addListener('place_changed', function() {
          infowindow.close();
          var place = autocomplete.getPlace();
          if (!place.geometry) {
            return;
        }

        if (place.geometry.viewport) {
            map.fitBounds(place.geometry.viewport);
        } else {
            map.setCenter(place.geometry.location);
            map.setZoom(17);
        }

          // Set the position of the marker using the place ID and location.
          marker.setPlace({
            placeId: place.place_id,
            location: place.geometry.location
        });
          marker.setVisible(true);

          infowindow.setContent('<div><strong>' + place.name + '</strong><br>' +
              'Place ID: ' + place.place_id + '<br>' +
              place.formatted_address);
          infowindow.open(map, marker);

          // write that destination in terminal
          $("#message").val("navigateTo(" + place.place_id + ")");
          $("#send").click();
      });
    }
</script>



</body>

'''