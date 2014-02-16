// JS code to control ultra logs. You should set the variable 'night'
// before loading this.

// initially logs are fully displayed
var show  = true;

function showFull() {
    // Shows class="full" table cell elements, hides class="brief"
    // elements. This allows the logs to change.
    $(".full").show();
    $(".brief").hide();
    show = true;
    return false;
}

function showBrief() {
    // Hides class="full" elements, shows class="brief"
    // elements. This allows the logs to change.
    $(".full").hide();
    $(".brief").show();
    show = false;
    return false;
}

function showTitleLog(night){
    // Show log of a night in the title & log sections
    var title = night + '/' + night + '_title.html';
    $("#titlecontent").load(title,function(){
        var log = night + '/' + night + '_log.html';
        $("#logcontent").load(log,function(){
            if(show){
                showFull();
            }else{
                showBrief();
            }
        });
    });
}

function showHideNights(run){
    // Hides/shows nights in a given run in the guide
    // Requires div elements with class="details" and IDs equal to
    // elements of the run class prefixed by guide_details_
    // which contain the night lists.
    var elem = document.getElementById('guide_details_' + run);
    var vis  = elem.style.display == "inline";

    // turn all off
    $(".details").hide();

    // switch specific one on if it was off
    if(!vis) elem.style.display = "inline";

    return false;
};

$(window).load(function(){

    $("#guidecontent").load('guide.html',function(){
        // load up guide, callback to load title and log in generic way

        // suppress all details
        $(".details").hide();

        // Load content as the value of 'night' or the
        // first night found in the guide if 'night' is
        // not set.
        console.log(night);
        var nset;
        if(night == "undef"){
            var elements = document.getElementsByClassName('night');
            nset = elements[0].id.substr(6);
        }else{
            nset = night;
        }
        showTitleLog(nset);

        $("#guidecontent").on('click','.night',function(evt){
            // Shows a night log linked in the guide section
            // in the main section
            evt.preventDefault();
            var night = this.id.substr(6);
            showTitleLog(night);
        });

    });

    // this for the previous / next links
    $("#titlecontent").on('click','.night',function(evt){
        evt.preventDefault();
        var night = this.id.substr(5);
        showTitleLog(night);
    });

});

