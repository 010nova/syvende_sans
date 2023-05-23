// get active spreasheet
var ss = SpreadsheetApp.getActiveSpreadsheet();

// get sheet named RawData
var sheet = ss.getSheetByName("Data");

var HEADER_ROW = 1;     // row index of header

function doPost(e) { 
  var cloudData = JSON.parse(e.postData.contents); // this is a json object containing all info coming from IoT Cloud
  var values = cloudData.values; // this is an array of json objects
  
  // read timestamp of incoming message
  var timestamp = values[0].updated_at;          // format: yyyy-MM-ddTHH:mm:ss.mmmZ
  var date = new Date(Date.parse(timestamp)); 
  
  // Get the values from the data JSON
  var innerValues = JSON.parse(values[0].value);
  var keys = Object.keys(innerValues);

  // Initialize the arrays
  var incLength = keys.length;
  var incNames = [];
  var incValues = [];

  for (var i = 0; i < incLength; i++) {
    incNames[i] = keys[i];
    incValues[i] = innerValues[keys[i]];
  }
  
  // Write out the dato/tid column labels
  for (var i = 0; i < incLength; i++) {
    sheet.getRange(HEADER_ROW, 1+i*3).setValue('Dato/Tid');
  }

  // Write the data column labels
  for (var i = 0; i < incLength; i++) {
    var col = 2+3*i; 
    // Check if the name is already in header
    if (sheet.getRange(HEADER_ROW, col).getValue() != incNames[i]) {
      sheet.getRange(HEADER_ROW, col).setValue(incNames[i]);
    }
  }
  
  // insert new row at the top in the columns where non-empty data is inserted if not duplicated date
  var duplicate = false;
  for (var i = 0; i < incLength; i++) {
    if(incValues[i]!="") {
      if(Date(sheet.getRange(HEADER_ROW+1, 1+3*i).getValue())!=date) {
        sheet.getRange(HEADER_ROW+1, 1+3*i).insertCells(SpreadsheetApp.Dimension.ROWS);
        sheet.getRange(HEADER_ROW+1, 2+3*i).insertCells(SpreadsheetApp.Dimension.ROWS);
      }
      else duplicate = true;
    }
  }
  
  if(!duplicate) {
    // reset style of the new row, otherwise it will inherit the style of the header row
    var range = sheet.getRange('A2:Z2');
    range.setFontColor('#000000');
    range.setFontSize(10);
    range.setFontWeight('normal');
    
    // write the timestamp where required
    for (var i = 0; i < incLength; i++) {
      if (incValues[i]!="") {
        sheet.getRange(HEADER_ROW+1, 1+i*3).setValue(date).setNumberFormat("yyyy-MM-dd HH:mm:ss");
      }
    }

    // write the values where required
    for (var i = 0; i < incLength; i++) {
      if (incValues[i]!="") {
        var col = 2+3*i;
        // first copy previous values
        // this is to avoid empty cells if not all properties are updated at the same time
        sheet.getRange(HEADER_ROW+1, col).setValue(sheet.getRange(HEADER_ROW+2, col).getValue()).setNumberFormat("0.00");

        // turn boolean values into 0/1, otherwise google sheets interprets them as labels in the graph
        if (incValues[i] == true) incValues[i] = 1;
        else if (incValues[i] == false) incValues[i] = 0;
        // fill the cell finally
        sheet.getRange(HEADER_ROW+1, col).setValue(incValues[i].replace(".", ",")).setNumberFormat("0.00");
      }
    }
  }

  for (var i = 0; i < incLength; i++) if (incValues[i]!="") removeDuplicates(i);
}

function removeDuplicates(i) {
  var lastRow = sheet.getLastRow();
  const data = sheet.getRange(2,1+i*3,lastRow-1,2).getValues();
  const uniqueData = {};
  for (let row of data) {
    const key = row[0];
    uniqueData[key] = [key, row[1]];
  }
  sheet.getRange(2, 1+i*3, lastRow-1, 2).clearContent();
  var newData = Object.values(uniqueData);
  sheet.getRange(2, 1+i*3, newData.length, 2).setValues(newData);
}