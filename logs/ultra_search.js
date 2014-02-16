/*
 * Script to enable dynamic web pages to search through JSON databases of
 * ultraspec and ultracam runs. This requires the JSON files created using
 * make_json.py in the logs directories.
 */

// swin used to refer to the search window
var swin;

// where json data goes
var data;

$(document).ready(function(){

    // Calling URL
    var instrument = "CAMERA";
    var url = document.URL;
    var nlogs = url.indexOf('/logs');
    if(nlogs > -1){
        var nslash = url.substring(0,nlogs).lastIndexOf('/');
        instrument = url.substring(nslash+1,nlogs).toUpperCase();
    }
    var elems = document.getElementsByClassName("camera");
    for(var n=0; n<elems.length; n++)
        elems[n].innerHTML = instrument;

    //  Loads the json file
    $.getJSON("ultra.json",
              function(data)
              {

                  // Compile array of unique target names and ras
                  var uniqID = [], info = [];
                  for (var i=0; i<data.length; i++){

                      // Create id as concatenation of name, ra, dec
                      id    = data[i].id + data[i].ra.toString() + data[i].dec.toString();
                      index = uniqID.indexOf(id);
                      name  = data[i].target.replace(/ /g,"~");
                      if (index < 0){
                          // store unique id and corresponding information line
                          uniqID.push(id);
                          info.push({id : data[i].id, ra : data[i].ra, dec : data[i].dec,
                                     names : [name]});
                      }else{
                          // old ID. Update target name list
                          if(info[index].names.indexOf(name) < 0)
                              info[index].names.push(name);
                      }
                  }

                  // sort by RA
                  info.sort(function(l, r){
                      return l.ra < r.ra ? -1 : 1;
                  });

                  // Create table for bottom of page

                  // header
                  var table = '<p>\nThere were ' + info.length + ' unique ID/RA/Dec combos.\n';
                  table += '\n<p>\n<table/>\n<tbody>\n<tr><th>ID</th><th>RA</th>' +
                      '<th>Dec</th><th class="left">Matching strings</th></tr>\n';

                  // contents
                  for (var i=0; i<info.length; i++){
                      table += '<tr><td class="left"><a class="tsearch" id="id' + i +
                          '" href="#">' + info[i].id + '</a>' +
                          '</td><td>' + to_dms(info[i].ra, 2, false) +
                          '</td><td>' + to_dms(info[i].dec, 1, true) +
                          '</td><td class="left">' + info[i].names.join(' ') +
                          '</td></tr>\n';
                  }

                  // finish table
                  table += "</tbody>\n</table>\n";

                  // stick in place
                  console.log(document.getElementById("targetTable"));
                  document.getElementById("targetTable").innerHTML = table;

                  console.log(document.getElementById("targetTable"));

                  // Creates and displays search results given central ra, dec (h,d),
                  // maximum distance, minium exposure
                  function createTable(ra0, dec0, maxrad, minexp){
                      var table = search_table_head(ra0,dec0,maxrad,minexp);
                      ra0 *= 15.;

                      for (var i=0; i<data.length; i++){
                          var ra     = 15.*data[i].ra;
                          var dec    = data[i].dec;
                          var expose = data[i].expose;
                          var dist   = Math.sqrt(Math.pow((ra-ra0)*
                                                          Math.cos(Math.PI*dec/180.),2)+
                                                 Math.pow(dec-dec0,2));
                          if(dist < maxrad && expose > minexp)
                              table += search_table_row(data[i], dist);
                      }
                      table += search_table_foot();

                      // send to a new window
                      // send to a different window
                      if(!swin || swin.closed){
                          swin = window.open("","_blank",
                                             "width=1100 height=500, resizable=yes, scrollbars=yes, menubar=yes");
                      }
                      swin.document.open();
                      swin.document.write(table);
                      swin.document.close();
                      swin.focus();
                  }

                  console.log(document.getElementById("targetTable"));

                  // Searches by RA and Dec extracted from page
                  $("#search").click(function(){
                      var ra0    = document.getElementById('ra').value;
                      var dec0   = document.getElementById('dec').value;
                      var maxrad = document.getElementById('dist').value;
                      var minexp = document.getElementById('expose').value;

                      createTable(ra0, dec0, maxrad, minexp);
                  });

                  // Searches by RA and Dec via id in link
                  $(".tsearch").click(function(evt){
                      evt.preventDefault();
                      document.getElementById('ra').value = info[this.id.substr(2)].ra;
                      document.getElementById('dec').value = info[this.id.substr(2)].dec;
                      var maxrad = document.getElementById('dist').value;
                      var minexp = document.getElementById('expose').value;

                      createTable(info[this.id.substr(2)].ra,
                                  info[this.id.substr(2)].dec, maxrad, minexp);
                  });
              });
});

function search_table_head(ra, dec, dist, expose){
    // Returns header for search results table
    var head =  '<!DOCTYPE html>\n' +
        '<html>\n' +
        '<head>\n' +
        '<script src="http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js">' +
        '</script>\n' +
        '<script>var init = true;</script>\n' +
        '<script src="ultra_logs.js">\n' +
        '</script>\n' +
        '<link rel="stylesheet" type="text/css" href="ultra.css" />\n' +
        '</head>\n' +
        '<body>\n' +
        '<h1>Run search results.</h1>\n';

    head += '<p>\n' +
        'Here are the runs on targets located within ' + dist +
        ' degrees on the sky\n' + 'of RA = ' + to_dms(ra,2,false) +
        ', Dec = ' + to_dms(dec,1,true) +
        ', and with an exposure time of at\n' +
        'least ' + expose + ' minutes.</p>\n<p>\n';

    head += "<table/>\n<tbody>\n<tr><th>Target</th><th>ID</th>" +
        "<th>RA<br>hours</th><th>Dec<br>deg.</th>" +
        "<th>Dist<br>deg.</th><th>Run</th><th>Night</th>" +
        "<th>Number</th><th>Expose<br>mins</th><th>Comment</th></tr>\n";
    return head;
}

function search_table_row(line, dist){
    // Returns one row of table data for search results
    return '<tr><td>'  + line.target +
        '</td><td>' + line.id +
        '</td><td>' + to_dms(line.ra, 2, false) +
        '</td><td>' + to_dms(line.dec, 1, true) +
        '</td><td>' + dist.toFixed(2) +
        '</td><td>' + line.run +
        '</td><td><a href="index.php?night=' + line.night + '">' + line.night +
        '</a></td><td>' + line.num +
        '</td><td>' + line.expose.toFixed(1) +
        '</td><td class="left">' + line.comment +
        '</td></tr>\n';
}

function search_table_foot(){
    foot = '</tbody>\n</table>\n\n<p>\n' +
        '"Dist" is the offset in degrees of the ID-ed position from the' +
        'search position.</p>\n\n<hr>\n' +
        '<address>\n' +
        'Tom Marsh, Warwick\n' +
        '</address>\n' +
        '</body>\n</html>';

    return foot;
}




