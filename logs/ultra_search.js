/*
 * Script to eanble dynamic web pages to search through JSON databases of
 * ultraspec and ultracam runs. This requires the JSON files created using
 * make_json.py in the logs directories.
 */

function to_dms (num, prec, sign) {
    // Expresses a number in degrees:minutes:seconds format
    // or equivalently hours:minutes:seconds. Can handle negative
    // numbers. Designed for 2-digits in leading number, i.e.
    // dd:mm:ss.ss or hh:mm:ss.ss
    //
    // num  -- the number
    // prec -- precision on the seconds
    // sign -- true to add leading sign
    var pos  = num >= 0.;
    var fd   = Math.abs(num)
    var id   = Math.floor (fd);
    var fm   = 60.*(fd-id);
    var im   = Math.floor(fm);
    var secs = (60.*(fm-im)).toFixed(prec);
    if(secs == 60.){
        im += 1;
        secs = 0.;
        if(im == 60.){
            id += 1;
            im = 0.;
        }
    }
    var ret = "";
    if(sign){
        if(pos){
            ret += "+";
        }else{
            ret += "-";
        }
    }
    if(id < 10) ret += "0";
    ret += id + ":";
    if(im < 10) ret += "0";
    ret += im + ":";
    if(secs < 10) ret += "0";
    ret += secs;
    return ret;
}

function search_table_head(ra, dec, dist, expose){
    // Returns header for search results table
    var head =  '<!DOCTYPE html>\n' +
        '<html>\n' +
        '<head>\n' +
        '<script src="http://ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js">' +
        '</script>\n' +
        '<script type="text/javascript" src="ultra_search.js">\n' +
        '</script>\n' +
        '<link rel="stylesheet" type="text/css" href="ultras.css" />\n' +
        '</head>\n' +
        '<body>\n' +
        '<h1>Run search results.</h1>\n';

    head += '<p>\n' +
        'Here are the runs on targets located within ' + dist + ' degrees on the sky\n' +
        'of RA = ' + to_dms(ra,2,false) + ', Dec = ' + to_dms(dec,1,true) +
        ', and with an exposure time of at\n' +
        'least ' + expose + ' minutes.</p>\n<p>\n';

    head += "<table/>\n<tr><th>Target</th><th>ID</th><th>RA<br>hours</th>" +
        "<th>Dec<br>deg.</th><th>Dist<br>deg.</th><th>Run</th><th>Night</th>" +
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
        '</td><td><a href="' + line.night + '/' + line.night + '.html">' + line.night +
        '</a></td><td>' + line.num +
        '</td><td>' + line.expose.toFixed(1) +
        '</td><td class="left">' + line.comment +
        '</td></tr>\n';
}

function search_table_foot(){
    foot = '</table>\n\n<p>\n' +
        '"Dist" is the offset in degrees of the ID-ed position from the search position.</p>\n\n' +
        '<hr>\n' +
        '<address>\n' +
        'Tom Marsh, Warwick\n' +
        '</address>\n' +
        '</body>\n</html>';

    return foot;
}

$(document).ready(function(){

        var swin;
        var wprops = "height=500,width=1050,scrollbars=yes,titlebar=yes";

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
                      var table = '<table/>\n<tr><th>ID</th><th>RA</th>' +
                          '<th>Dec</th><th class="left">Matching strings</th></tr>\n';

                      // contents
                      for (var i=0; i<info.length; i++){
                          table += '<tr><td class="left"><a class="tsearch" id="' + i + 
                              '" href="#">' + info[i].id + '</a>' +
                              '</td><td>' + to_dms(info[i].ra, 2, false) +
                              '</td><td>' + to_dms(info[i].dec, 1, true) +
                              '</td><td class="left">' + info[i].names.join(' ') +
                              '</td></tr>\n';
                      }

                      // finish table
                      table += "</table>\n";

                      // stick in place
                      document.getElementById("targetTable").innerHTML = table;

                      // Searches by RA and Dec extracted from page
                      $("#search").click(function(){
                              var ra0    = 15.*document.getElementById('ra').value;
                              var dec0   = document.getElementById('dec').value;
                              var maxrad = document.getElementById('dist').value;
                              var minexp = document.getElementById('expose').value;

                              // create results table
                              var table = search_table_head(document.getElementById('ra').value,
                                                            dec0,maxrad,minexp);

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

                              // send to a different window
                              if(!swin)
                                  swin = window.open("","Search results",wprops);
                              swin.document.open();
                              swin.document.write(table);
                              swin.document.close();
                          });

                      // Searches by RA and Dec via id in link
                      $(".tsearch").click(function(evt){
                              evt.preventDefault();
                              document.getElementById('ra').value = info[this.id].ra;
                              document.getElementById('dec').value = info[this.id].dec;
                              var ra0    = 15.*info[this.id].ra;
                              var dec0   = info[this.id].dec;
                              var maxrad = document.getElementById('dist').value;
                              var minexp = document.getElementById('expose').value;

                              var table = search_table_head(info[this.id].ra,dec0,maxrad,minexp);

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
                              if(!swin)
                                  swin = window.open("","Search results",wprops);
                              swin.document.open();
                              swin.document.write(table);
                              swin.document.close();
                          });
                  });
    });


