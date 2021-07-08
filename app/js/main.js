var imageOriginal = new MarvinImage();
var imageProcessed = new MarvinImage();
var imageDisplay = new MarvinImage();
var threshold = [];
var name;
var reader;
var form;
var cords = {};


$('#fileUpload').on('change', function () {
	

	name = event.target.files[0].name;
	reader = new FileReader();
	reader.readAsDataURL(event.target.files[0]);
	
	reader.onload = function(){
		imageOriginal.load(reader.result, imageLoaded);
		imageProcessed = imageOriginal.clone();

	};
	
});

$('#color').on('change', function () {
	var c = $(this).find('option:selected').val()
	$('#divc').css("background-color", c) ;
//	alert($(this).find('option:selected').val());
	

});


function imageLoaded() {
	var xtop = 1200
	var ytop = 1700

	//crops image as relavant info is at the middle. Better visualization of the spot
	if (getCookie("xtop")) xtop=parseInt(getCookie("xtop"))
	if (getCookie("ytop")) ytop=parseInt(getCookie("ytop"))
	
	
	Marvin.crop(imageOriginal, imageProcessed, xtop, ytop, 500, 500);
	
	repaint();	
}

function repaint() {
	var canvas = document.getElementById("canvas");
	var divc= document.getElementById("divc");
	var ctx = canvas.getContext('2d');
	var imageratio;

	ctx.fillStyle = "#eeeeee";
    
	

	x = imageProcessed.getWidth();
	y = imageProcessed.getHeight();
	imageratio = x / y;
	
    // for making the image smaller. This line is just for the sake of changing the crop...but not necessary at this point
	if (x > 500) { x = 500; y = x / imageratio } 

	canvas.width = x;
	canvas.height = y;
	

	divc.style.width = x+ "px";
	divc.style.height = y + "px";

	ctx.drawImage(canvas, 0, 0, x, y);
	Marvin.scale(imageProcessed, imageDisplay, x);

	imageDisplay.draw(canvas);
	imageProcessed.update();

}

function analyse(blank=0) {
	/*
	analyses the image inside a give threshold defined by the blank,

	varialbes explanation

	thres - defines a minimum value to be analysed, e.g, only values above 10 of intensity will be analysed. At this point it should not be necessary as we are only analysing inside the mask defined by the blank.
	ri,gi,bi - each color integration value
	undersat - min value for blank define undersaturation. Only important when analysing the blank images
	usat - related with undersat, it is a counter for the number of times it permits the undersat values
	integration - a sum of the ri, bi, and bi. This is the total final numeric value
	sat - number of allowed pixels that can be saturated. Eventually should be a fraction of the thresold matrix but as the size is approx always the same, this is not important
	divf (beta) - dividing factor in the case we are analysing a single color for having the same value if 2 or 3 colores analysed.
	*/
	
	var ri = 0;
	var gi = 0;
	var bi = 0;
	var thres = 10;
	var divf = 255;		
	var maxsat = 3000;
	var sat = 0;
	var undersat = 200;
	var usat = 0;
	var n = 0;
	var integration = 0;
	
	if (getCookie("maxsat"))  maxsat=getCookie("maxsat")
	if (noimage()) return false;					
	for (index = 0; index < threshold.length; ++index) {

		x = threshold[index].x;
		y = threshold[index].y;
		var r = imageDisplay.getIntComponent0(x, y);
		var g = imageDisplay.getIntComponent1(x, y);
		var b = imageDisplay.getIntComponent2(x, y);
		if (r > thres) { ri = ri + r; n++; if (r > undersat) usat++; } // under exposure
		if (g > thres) { gi = gi + g; n++; if (g > undersat) usat++; }
		if (b > thres) { bi = bi + b; n++; if (b > undersat) usat++; }
		if (r > 254 || g > 254 || b > 254) sat++;
	}

	var e = document.getElementById("color");
	var c = e.options[e.selectedIndex].value;
	if (c == "red") { gi = 0; bi = 0; }
	if (c == "green") { ri = 0; bi = 0 }
	if (c == "blue") { ri = 0; gi = 0 }
	if (c == "yellow") { bi = 0; divf = divf + divf } //just for the au to be more or less in the same order of magnitude
	if (c == "grey") { bi = 0; divf = divf + divf + divf }			//just for the au to be more or less in the same order of magnitude

	//alert(divf+c);

	integration = ri + gi + bi;

	if (sat > maxsat) { alert("Over " + sat + " pixels saturated (maximum defined " + maxsat + "). Consider decreasing the exposure time"); return false}
	if ((blank) && (usat<10)) { alert("Integration=" + usat + " . Blank image too dim. Consider increasing the exposure time"); return false } //if also of the pixels are under saturated
	
	return integration / divf;

	repaint();
}

function blank() {			
		
	if (noimage()) return false;

	//for re-blank we need to clean the thresold values
	threshold = [];
	var cords = {};

	//erosing and inflating image to be more homogenious and eventually to fill holes. morphologicalErosion and morphologicalDilation can be used more than once.
	//Eventually in a loop...but it looks that making the process once is enough
	var bin = MarvinColorModelConverter.rgbToBinary(imageDisplay, 145);
	Marvin.morphologicalErosion(bin);
	Marvin.morphologicalDilation(bin);
	imageProcessed = MarvinColorModelConverter.binaryToRgb(bin);		
	
	for (var y = 0; y < bin.getHeight(); y++) {	    
		for (var x = 0; x < bin.getWidth(); x++) {
		
			if (imageProcessed.getIntComponent0(x, y) == 255) {
					// if thresholded, all colors are max as the image is now binary. Value are pushed to threshould array as object {x: value, y: value}
					threshold.push({ x, y });
				}	
			}
	}

	// analyse image that originate the threshold to check wheter exposure is good
	var res = analyse(1);		
	if (res) {

		res = res.toFixed(0);
		alert("Blank value of " + res + "a.u. \nNew measuring coordinates locally stored");

		//store threshold locally
		putThreshold();

		//store black as cookie. Made as cookie as it is easier then to retrieve all storared local values to threshold, just that
		document.cookie = "blank=" + res;

		
		repaint();
	} else {

		return false;
    }
}

function measure() {
	var blank_ = getCookie("blank");

	//needs to have both the black value to check intensities and measuring matrix
	
	if (!(getThreshold())) { alert("No measuring matrix found.\nPlease blank for creating measuring matrix first"); return false };
	
	if (!(blank_)) { alert("No blank value found. Please blank before proceed"); return false };
		
	var res = analyse();
	if (res) {
		if (blank_ < res) {	("Blank value smaller than sample. Please repeat blanking"); return };
		var t = Math.log10(blank_ / res);
		t = t.toFixed(3);		
		alert(t + " OD");
		putValues(t);

    }
}

function getCookie(cname) {
	//standard function to read a cookie

	var name = cname + "="; 
	var ca = document.cookie.split(';');
	for (var i = 0; i < ca.length; i++) {
		var c = ca[i];
		while (c.charAt(0) == ' ') {
			c = c.substring(1);
		}
		if (c.indexOf(name) == 0) {
			return c.substring(name.length, c.length);
		}
	}
	return false;
}

function noimage() {
	if (!(imageDisplay.getHeight())) {
		alert("Image not selected");
		return true;
	}
}

function putThreshold() {
	//store thresold locally. Max size 5 Mb...a cookie is 4kb and might no be enough

	localStorage.clear();
	for (index = 0; index < threshold.length; ++index) {
		x = threshold[index].x;
		y = threshold[index].y;
		cords = { x, y };
		localStorage.setItem(index, JSON.stringify(cords));	
	}
}

function putValues(value) {

	localStorage.setItem("___" + name, value);	
}

function getThreshold() {
	//reads the threshold from local storage

	threshold = [];
	t = JSON.parse(JSON.stringify(localStorage))

	for ([key, cords] of Object.entries(t)) { // transforms key into array
		cords = JSON.parse(cords);
		x = parseInt(cords.x);
		y = parseInt(cords.y);
		threshold.push({ x, y });
	}		
		
	
	if (threshold.length == 0) return false;

	return threshold;

}

function dload() {
	csvContent = "";
	t = JSON.parse(JSON.stringify(localStorage))
	for ([key, val] of Object.entries(t)) {
		if (key.substring(0, 3) == "___") { csvContent = csvContent+ key + "," + val + "\n" };
    }

	var a = document.createElement('a');
	mimeType = 'text/csv;encoding:utf-8'
	a.href = URL.createObjectURL(new Blob([csvContent ], {
		type: mimeType
	}));
	a.setAttribute('download', "ODsdata.csv");
	document.body.appendChild(a);
	a.click();
	document.body.removeChild(a);
	
}
			
