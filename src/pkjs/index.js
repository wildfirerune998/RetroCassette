// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config.json');
// Initialize Clay
var clay = new Clay(clayConfig);

var era = "";
var show_weekday_style = "";

function sendToPebble() {
         
    // Assemble dictionary using our keys
    var dictionary = {
    "ERA": era,
    "SHOWWEEKDAYSTYLE": show_weekday_style
    };

    // Send to Pebble
    Pebble.sendAppMessage(dictionary, function(e) {
    console.log('info sent to Pebble successfully!');
    },
    function(e) {
        console.log('Error sending info to Pebble!');
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
        console.log('OH FALSE');
      }
     
    );  }
);
// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {

    if (!e.payload || typeof e.payload === undefined){
        return;
    }

    console.log('addEventListener appmessage e.response ' + JSON.stringify(e.payload));
    var era_string = "";
    var show_weekday_style_string ="";

    era_string = JSON.stringify(e.payload.ERA);
    console.log('appmessage era_string ' + era_string);
    if (era_string) {
        era = era_string;
    }

    show_weekday_style_string = JSON.stringify(e.payload.SHOWWEEKDAYSTYLE);
    if (!show_weekday_style_string){

    }
    console.log('appmessage show_weekday_style_string ' + show_weekday_style_string);
    show_weekday_style = show_weekday_style_string.toString();

    sendToPebble();
  }                  
);
var messageKeys = require('message_keys');

Pebble.addEventListener('webviewclosed', function(e) {
  
  console.log('addEventListener webviewclosed e.response ' + e.response);
  if (e && !e.response) {
    return;
  };
  
  // Get the keys and values from each config item
  var claySettings = clay.getSettings(e.response);
  var era_string = "";
  var show_weekday_style_string ="";
  
  era_string = claySettings[messageKeys.ERA];
  show_weekday_style_string = claySettings[messageKeys.SHOWWEEKDAYSTYLE];

  if (era_string) {
    era = era_string;
  }

  console.log('addEventListener webviewclosed show_weekday_style_string ' + show_weekday_style_string);
  show_weekday_style = show_weekday_style_string.toString();
  
  sendToPebble(); 

}
);