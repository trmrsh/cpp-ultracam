// JS for controlling the ULTRASPEC/CAM logs and guide. Combined with
// ultra.css this allows for display of the right bit in the right place,
// and selective hiding and showing of elements.
//
// TRM Jan 2014. Uses jquery.

// initially logs are fully displayed
var show = true;

// shows/hides class=full elements, and the reverse on
// class=brief elements
function showHide() {
    if(show){
        $('.full').show();
        $('.brief').hide();
    }else{
        $('.full').hide();
        $('.brief').show();
    }
}

// Show log of a night in the main section
function showMain(night){
    var url = night + '/' + night + '.html';
    $("#maincontent").load(url,function(){
            showHide();
        });
}

$(window).load(function(){

        // Shows a night log linked in the main section in the main section
        // IDs prefixed by '_' to make unique.
        $("#maincontent").on('click','.night',function(evt){
                evt.preventDefault();
                var night = this.id.substr(1);
                showMain(night);
            });

        // Shows a night log linked in the guide section in the main section
        $("#guidecontent").on('click','.night',function(evt){
                evt.preventDefault();
                var night = this.id;
                showMain(night);
            });

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

        // Shows the search page in the main section
        $("#guidecontent").on('click','.search',function(evt){
                evt.preventDefault();
                $("#maincontent").load("ultra_search.html");
            });

        // Hides all "full" class elements
        $("#guidecontent").on('click', "#short", function(evt){
                evt.preventDefault();
                show = false;
                showHide();
            });

        // Shows all "full" class elements
        $("#guidecontent").on('click', "#long", function(evt){
                evt.preventDefault();
                show = true;
                showHide();
            });

    });


$(document).ready(function() {

        // Load guide
        $("#guidecontent").load('guide.html',function(){
                // Load main content as first night found
                var elements = document.getElementsByClassName('night');
                var night = elements[0].id;
                showMain(night);
            });

    });



