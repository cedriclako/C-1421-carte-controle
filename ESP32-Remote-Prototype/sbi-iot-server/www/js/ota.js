function idBtnUpload_Click()
{
	let lblStatus = document.getElementById("lblStatus");

	try {
		console.log("idBtnUpload_Click");
		var xhr = new XMLHttpRequest();
		xhr.open('POST', '/ota/upload_esp32', true);

		let firmwareFile = document.getElementById("idFile").files[0];

		// Listen to the upload progress.
		xhr.upload.onprogress = function(e)
		{
			console.log("upload in onprogress, loaded: ", e.loaded, ", total: ", e.total);
			lblStatus.innerHTML = "Uploading (" + ((e.loaded / e.total)*100).toFixed(2) + "%)";
		};

		xhr.onreadystatechange = function (e) {
			if (xhr.readyState === 4) {
				if (xhr.status === 200) {
					console.log("Upload succeeded!", e);
					lblStatus.innerHTML = "Upload completed!";
				} else {
				   console.log("Error: ", e);
				   lblStatus.innerHTML = "Upload error: " + xhr.statusText;
				}
			}
		};

		lblStatus.innerHTML = "Uploading ...";
		xhr.send(firmwareFile);
	}
	catch(err) {
		lblStatus.innerHTML = "Upload error: " + err.message;
	}
}