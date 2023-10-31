function idBtnUploadESP32_Click()
{
	let lblStatusESP32 = document.getElementById("lblStatusESP32");

	try {
		console.log("idBtnUpload_Click");
		var xhr = new XMLHttpRequest();
		xhr.open('POST', API_POST_ESP32_URI, true);

		let firmwareFile = document.getElementById("idFileESP32").files[0];

		// Listen to the upload progress.
		xhr.upload.onprogress = function(e)
		{
			console.log("upload in onprogress, loaded: ", e.loaded, ", total: ", e.total);
			lblStatusESP32.innerHTML = "Uploading (" + ((e.loaded / e.total)*100).toFixed(2) + "%)";
		};

		xhr.onreadystatechange = function (e) {
			if (xhr.readyState === 4) {
				if (xhr.status === 200) {
					console.log("Upload succeeded!", e);
					lblStatusESP32.innerHTML = "Upload completed!";
				} else {
				   console.log("Error: ", e);
				   lblStatusESP32.innerHTML = "Upload error: " + xhr.statusText;
				}
			}
		};

		lblStatusESP32.innerHTML = "Uploading ...";
		xhr.send(firmwareFile);
	}
	catch(err) {
		lblStatusESP32.innerHTML = "Upload error: " + err.message;
	}
}

function idBtnUploadSTM32_Click()
{
	let lblStatusSTM32 = document.getElementById("lblStatusSTM32");

	try {
		console.log("idBtnUpload_Click");
		var xhr = new XMLHttpRequest();
		xhr.open('POST', API_POST_STM32_URI, true);

		let firmwareFile = document.getElementById("idFileESP32").files[0];

		// Listen to the upload progress.
		xhr.upload.onprogress = function(e)
		{
			console.log("upload in onprogress, loaded: ", e.loaded, ", total: ", e.total);
			lblStatusSTM32.innerHTML = "Uploading (" + ((e.loaded / e.total)*100).toFixed(2) + "%)";
		};

		xhr.onreadystatechange = function (e) {
			if (xhr.readyState === 4) {
				if (xhr.status === 200) {
					console.log("Upload succeeded!", e);
					lblStatusSTM32.innerHTML = "Upload completed!";
				} else {
				   console.log("Error: ", e);
				   lblStatusSTM32.innerHTML = "Upload error: " + xhr.statusText;
				}
			}
		};

		lblStatusSTM32.innerHTML = "Uploading ...";
		xhr.send(firmwareFile);
	}
	catch(err) {
		lblStatusSTM32.innerHTML = "Upload error: " + err.message;
	}
}
