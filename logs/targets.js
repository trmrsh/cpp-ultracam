/*
 * Script to display complete list of ULTRACAM / SPEC targets
 */

// where json data goes
var data;

$(document).ready(function(){

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

                      // Create table

                      // header
                      var table = '<p>\nThere are ' + info.length + ' unique ID/RA/Dec combos.\n';
                      table += '\n<p>\n<table/>\n<tbody>\n<tr><th>ID</th><th>RA</th>' +
                          '<th>Dec</th><th class="left">Matching strings</th></tr>\n';

                      // contents
                      for (var i=0; i<info.length; i++){
                          table += '<tr><td class="left">' + info[i].id +
                              '</td><td>' + to_dms(info[i].ra, 2, false) +
                              '</td><td>' + to_dms(info[i].dec, 1, true) +
                              '</td><td class="left">' + info[i].names.join(' ') +
                              '</td></tr>\n';
                      }

                      // finish table
                      table += "</tbody>\n</table>\n";

                      // stick in place
                      document.getElementById("targets").innerHTML = table;
                  });
    });
