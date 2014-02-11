// JS for controlling the ULTRASPEC/CAM logs and guide. Combined with
// ultra.css this allows for display of the right bit in the right place,
// and selective hiding and showing of elements.
//
// TRM Jan 2014. Uses jquery.

$(document).ready(function() {

        // initially logs are fully displayed
        var full = true;

        // Load guide
        $("#guidecontent").load('guide.html');

        // Load (initial) main content
        $("#maincontent").load('2013-11-05/2013-11-05.html');

        // Hides/shows nights in a given run in the guide
        // Requires div elements with IDs equal to
        // elements of the run class plus "_details"
        // which contain the night lists.
        $("#guidecontent").on('click','.run',function(evt){
                evt.preventDefault();
                var elem = '#' + this.id + '_details';
                var vis = $(elem).is(":visible");
                $('.details').hide();
                if(!vis) $(elem).show();
            });

        // Shows a night log linked in the guide section in the main section
        $("#guidecontent").on('click','.fnight',function(evt){
                evt.preventDefault();
                var night = this.id;
                var url = night + '/' + night + '.html';
                $("#maincontent").load(url);
                if(full){
                    $('.full').show();
                }else{
                    $('.full').hide();
                }
            });

        // Shows a night log linked in the main section in the main section
        // IDs prefixed by '_' to make unique.
        $("#maincontent").on('click','.fnight',function(evt){
                evt.preventDefault();
                var night = this.id.substr(1);
                var url = night + '/' + night + '.html';
                $("#maincontent").load(url);
                if(full){
                    $('.full').show();
                }else{
                    $('.full').hide();
                }
            });

        // Shows the search page in the main section
        $("#guidecontent").on('click','.query',function(evt){
                evt.preventDefault();
                $("#maincontent").load("ultraspec_query.html");
            });

        // Hides all "full" class elements
        $("#guidecontent").on('click', "#short", function(evt){
                evt.preventDefault();
                $('.full').hide();
                full = false;
            });

        // Shows all "full" class elements
        $("#guidecontent").on('click', "#long", function(evt){
                evt.preventDefault();
                $('.full').show();
                full = true;
            });
    });
