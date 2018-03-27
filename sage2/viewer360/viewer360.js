// SAGE2 is available for use under the SAGE2 Software License
//
// University of Illinois at Chicago's Electronic Visualization Laboratory (EVL)
// and University of Hawai'i at Manoa's Laboratory for Advanced Visualization and
// Applications (LAVA)
//
// See full text, terms and conditions in the LICENSE.txt included file
//
// Copyright (c) 2015

//
// original SAGE implementation: Toan Nguyen
//


"use strict";

/* global THREE */


/**
 * WebGL 3D application, inherits from SAGE2_WebGLApp
 *
 * @class car_threejs
 */
var viewer360 = SAGE2_WebGLApp.extend({

	init: function(data) {
		// Create a canvas into the DOM
		this.WebGLAppInit('canvas', data);

		// Set the background to black
		this.element.style.backgroundColor = 'black';

		this.renderer = null;
		this.camera   = null;
		this.scene    = null;
		this.ready    = null;

		this.element.id = "div" + data.id;
		this.frame  = 0;
		this.width  = this.element.clientWidth;
		this.height = this.element.clientHeight;
		this.dragging = false;
		this.ready    = false;
		
		this.usePhoto = false;
		this.isPhoto = false;
		this.isVideo = false;
		this.photoSrc = null;
		this.videoSrc = null;
		this.texture = null;
		this.mesh = null;

		this.lat = 0;
        this.lon = 0;
        this.fov = 35;
        this.flatProjection = false;
        this.prevX = 0;
        this.prevY = 0;

		// build the app
		this.initialize(data.date);

		this.resizeCanvas();
		this.refresh(data.date);
	},

	initialize: function(date) {
		console.log("initialize...");
		// CAMERA
		this.camera = new THREE.PerspectiveCamera(this.fov, this.width / this.height, 0.1, 1000);
		this.camera.position.set(0, 0, 0);

		// SCENE
		this.scene = new THREE.Scene();

		if (this.renderer == null) {
			this.renderer = new THREE.WebGLRenderer({
				canvas: this.canvas,
				antialias: true
			});
			this.renderer.setSize(this.width, this.height);
			this.renderer.autoClear = false;
			this.renderer.setClearColor( 0x333333, 1 );

			this.renderer.gammaInput  = true;
			this.renderer.gammaOutput = true;
		}

		if(this.usePhoto){
			this.photoSrc = this.resrcPath + '/testdata/testpic.jpg';
			this.isPhoto = true;
			this.texture = THREE.ImageUtils.loadTexture( this.photoSrc );
			console.log(this.photoSrc);
		}
		else {
			this.isVideo = true;
            this.video = document.createElement( 'video' );
            this.video.src = this.resrcPath + '/testdata/testvideo.mp4';
            this.video.style.display = 'none';
            this.div = document.getElementById(this.element.id);
            this.div.appendChild( this.video );
            this.video.loop = true;
            this.video.muted = true;
            this.video.autoplay = true;
            this.texture = new THREE.Texture( this.video );
		}
           
        this.texture.generateMipmaps = false;
        this.texture.minFilter = THREE.LinearFilter;
        this.texture.magFilter = THREE.LinearFilter;
        this.texture.format = THREE.RGBFormat;

        this.mesh = new THREE.Mesh( new THREE.SphereGeometry( 500, 80, 50 ), new THREE.MeshBasicMaterial( { map: this.texture } ) );
        this.mesh.scale.x = -1; // mirror the texture
        this.scene.add(this.mesh);

		this.ready = true;

		// draw!
		this.resize(date);
	},

	load: function(date) {
		// nothing
	},

	draw: function(date) {
		if (this.ready) {	
			this.lat = Math.max( - 85, Math.min( 85, this.lat ) );
            this.phi = ( 90 - this.lat ) * Math.PI / 180;
            this.theta = this.lon * Math.PI / 180;

            var cx = 500 * Math.sin( this.phi ) * Math.cos( this.theta );
            var cy = 500 * Math.cos( this.phi );
            var cz = 500 * Math.sin( this.phi ) * Math.sin( this.theta );

            this.camera.lookAt(new THREE.Vector3(cx, cy, cz));

            // distortion
            if(this.flatProjection) {
                this.camera.position.x = 0;
                this.camera.position.y = 0;
                this.camera.position.z = 0;
            } else {
                this.camera.position.x = - cx;
                this.camera.position.y = - cy;
                this.camera.position.z = - cz;
            }

            if( this.isVideo ) {
                if ( this.video.readyState === this.video.HAVE_ENOUGH_DATA) {
                    if(typeof(this.texture) !== "undefined" ) {
                    	this.texture.needsUpdate = true;
                    }
                }                
            }

			this.renderer.clear();
			this.renderer.render(this.scene, this.camera);
		}
	},

	// Local Threejs specific resize calls.
	resizeApp: function(resizeData) {
		if (this.renderer != null && this.camera != null) {
			this.renderer.setSize(this.canvas.width, this.canvas.height);
			this.camera.setViewOffset(this.sage2_width, this.sage2_height,
									resizeData.leftViewOffset, resizeData.topViewOffset,
									resizeData.localWidth, resizeData.localHeight);
		}
	},

	event: function(eventType, position, user_id, data, date) {

		if (this.ready) {
			if (eventType === "pointerPress" && (data.button === "left")) {
				this.dragging = true;
				this.prevX = position.x;
				this.prevY = position.y;
	
			} else if (eventType === "pointerMove" && this.dragging) {
				this.lon += position.x - this.prevX;
                this.lat -= position.y - this.prevY;
                this.prevX = position.x;
                this.prevY = position.y;
				this.refresh(date);

			} else if (eventType === "pointerRelease" && (data.button === "left")) {
				this.dragging = false;
			}

			if (eventType === "pointerScroll") {
				var wheelSpeed = -0.01;

				if(data.wheelDelta)
					this.fov -= data.wheelDelta * wheelSpeed;

	            var fovMin = 3;
	            var fovMax = 100;

	            if(this.fov < fovMin) {
	                this.fov = fovMin;
	            } else if(this.fov > fovMax) {
	                this.fov = fovMax;
	            }

	            this.camera.setLens(this.fov);
	            this.refresh(date);
			}
		}
	}

});
