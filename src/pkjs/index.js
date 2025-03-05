// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config.json');
// Initialize Clay
var clay = new Clay(clayConfig);

var era = "";

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function sendToPebble() {
         
    // Assemble dictionary using our keys
    var dictionary = {
    "ERA": era
    };

    // Send to Pebble
    Pebble.sendAppMessage(dictionary, function(e) {
    //console.log('era info sent to Pebble successfully!');
    },
    function(e) {
        console.log('Error sending era info to Pebble!');
    });//sendAppMessge
};

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function(e) {

    var dictionary = {
      "READY": 1
    };

    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log('READY');
      },
      function(e) {
        console.log('OH NO');
      }
     
    );  }
);
// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {

    if (!e.payload){
        return;
    }

    //console.log('e.response ' + e.payload);
    var era_string;

    era_string = JSON.stringify(e.payload.ERA);
    if (era_string) {
        era = era_string;
    }
    sendToPebble();
  }                  
);
var messageKeys = require('message_keys');

Pebble.addEventListener('webviewclosed', function(e) {
  
  //console.log('e.response ' + e.response);
  if (e && !e.response) {
    return;
  };
  
  // Get the keys and values from each config item
  var claySettings = clay.getSettings(e.response);
  var era_string;
  
  era_string = claySettings[messageKeys.ERA];

  if (era_string) {
    era = era_string;
  }
  sendToPebble(); 

}
);