
// Get the modal
var modal = document.getElementById("myModal");

// Get the button objects
var set = document.getElementById("settings");
var blk = document.getElementById("blank");
var mea= document.getElementById("measure");
var dld= document.getElementById("download");
var sto= document.getElementById("store");
// Get the <span> element that closes the modal
var span = document.getElementsByClassName("close")[0];

// actions
set.onclick = () => {
  modal.style.display = "block"
  if (getCookie("xtop"))  document.getElementById("xtop").value=getCookie("xtop")
  if (getCookie("ytop"))  document.getElementById("ytop").value=getCookie("ytop")
  if (getCookie("maxsat"))  document.getElementById("maxsat").value=getCookie("maxsat")
}
blk.onclick = ()=> {blank()}
mea.onclick = ()=> {measure()}
dld.onclick = ()=> {dload()}
sto.onclick = ()=> {
  document.cookie = "xtop=" + document.getElementById("xtop").value
  document.cookie = "ytop=" + document.getElementById("ytop").value
  document.cookie = "maxsat=" + document.getElementById("maxsat").value
  alert("Settings recorded!")
  location.reload()
}



// When the user clicks on <span> (x), close the modal
span.onclick = ()=> {modal.style.display = "none"}

// When the user clicks anywhere outside of the modal, close it
window.onclick = function(event) {
  if (event.target == modal) {
    modal.style.display = "none";
  }
}

