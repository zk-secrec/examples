import { Injectable } from '@angular/core';
import * as L from 'leaflet';
import 'leaflet-editable';
import proj4 from 'proj4';
import {BehaviorSubject, windowToggle} from 'rxjs';
import 'leaflet.offline';


@Injectable({
  providedIn: 'root'
})
export class MapService {

  map?: L.Map;
  path?: L.Polyline;

  private pathDistance = new BehaviorSubject<number>(0);
  pathDistance$ = this.pathDistance.asObservable();
  private pathInBounds = new BehaviorSubject<number>(100);
  pathInBounds$ = this.pathInBounds.asObservable();

  private circles_gps: Array<L.LatLng>;
  
  private circles = [
    [1289637,434770],
    [1289207,445938],
    [1277416,445590],
    [1277587,455568],
    [1267011,446715],
  ];

  private circle_radii = [
    3584.29,
    1528.87,
    3283.96,
    1417.12,
    953.30,
  ];

  constructor() {
    proj4.defs(
      "EPSG:2248",
      "+proj=lcc +lat_1=39.45 +lat_2=38.3 +lat_0=37.66666666666666 +lon_0=-77 +x_0=399999.9998983998 +y_0=0 +ellps=GRS80 +datum=NAD83 +to_meter=0.3048006096012192 +no_defs "
    );
    this.circles_gps = this.circles.map(x => L.latLng(proj4("EPSG:2248").inverse(x).reverse() as L.LatLngTuple));
  }

  initializeLeafletMap(): void {
    // Base map view
    this.map = L.map('map', {
      editable: true,
      center: [58.7, 25],
      zoom: 8
    });
    const tiles = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        crossOrigin: "anonymous",
        maxZoom: 19,
        attribution: '© OpenStreetMap'
    })
    tiles.addTo(this.map);

    this.map.setView(new L.LatLng(38.860454,-77.079703), 12);

    // Draw bounds
    var circle_pane = this.map.createPane('semitransparent');
    circle_pane.style.opacity = '0.2';
    var canvasRenderer = L.canvas({pane: 'semitransparent'});

    // Outer rectangle
    L.rectangle([[-180,-180], [180,180]], {
      stroke: false,
      fillColor: "#ff0000",
      fillOpacity: 0.7,
      renderer: canvasRenderer
    }).addTo(this.map);
    // Circles
    for (let i = 0; i < this.circles_gps.length; i++) {
      L.circle(this.circles_gps[i], {
        radius: this.circle_radii[i],
        stroke: false,
        fillColor: '#ffffff',
        fillOpacity: 1.0,
        renderer: canvasRenderer
      }).addTo(this.map);
    }
    this.resetPath();
    this.map.addEventListener("editable:drawing:clicked", this.updateStats.bind(this));
    this.map.addEventListener("editable:vertex:dragend", this.updateStats.bind(this));

    var resetControl = L.Control.extend({
      options: {position: 'topleft', context: this},
      onAdd: function() {
        var container = L.DomUtil.create('div', 'leaflet-control leaflet-bar'),
            link = L.DomUtil.create('a', '', container);
        link.title = "Clear path";
        link.innerHTML = "<span class=\"fa-solid fa-repeat\"></span>";
        L.DomEvent
          .on(link, 'click', L.DomEvent.stop)
          .on(link, 'click', () => {this.options.context.resetPath()});
        return container;
      },
    });

    const temp = this.map;
    var darpa_pentagon_paris = [
      [1281189,441367],
      [1281284,441831],
      [1282387,442136],
      [1283039,442595],
      [1283305,442757],
      [1284203,443442],
      [1284767,443496],
      [1285192,443504],
      [1285529,443513],
      [1285816,443507],
      [1285848,443502],
      [1286426,442451],
      [1287273,441271],
      [1288440,440214],
      [1289452,438661],
      [1290971,437049],
      [1292740,436441],
      [1294430,436916],
      [1295619,436665],
      [1295617,436654],
      [1295563,436300],
      [1295380,435743],
      [1295265,435669],                 
  ]
    window.points = darpa_pentagon_paris;

    window.refreshPoints = () => {
      
      var mapped = window.points.map((x : number[]) => L.latLng(proj4("EPSG:2248").inverse(x).reverse() as L.LatLngTuple));
      if (this.path !== undefined) {
        this.path.remove();
      }
      this.path = L.polyline(mapped, {color: 'red'}).addTo(temp);
      temp.fitBounds(this.path.getBounds());
      window.dispatchEvent(new Event('resize'));
      this.updateStats();
    };

    window.refreshPoints();
  }

  resetPath(): void {
    if (this.map !== undefined) {
      if (this.path !== undefined)
        this.path.remove();
      this.pathDistance.next(0);
      this.pathInBounds.next(100);
    }
  }

  updateStats(): void {
    if (this.path === undefined) return;
    var lastMarker!: L.LatLng;
    var lastMarkerInBounds: boolean = true;
    var distWithinBounds: number = 0;
    var distOutsideBounds: number = 0;
    for (var marker of this.path.getLatLngs()) {
      marker = marker as L.LatLng;
      var markerInBounds = false;
      for (var i = 0; i < this.circles_gps.length; i++) {
        if (inCircle(this.circles_gps[i], marker, this.circle_radii[i])) {
          markerInBounds = true;
          break;
        }
      }
      if (lastMarker) {
        const dist = lastMarker.distanceTo(marker);
        markerInBounds && lastMarkerInBounds ?
          distWithinBounds += dist :
          distOutsideBounds += dist;
      }
      lastMarkerInBounds = markerInBounds;
      lastMarker = marker;
    }
    this.pathDistance.next(distOutsideBounds + distWithinBounds);
    this.pathInBounds.next(
      (distWithinBounds * 100 / (distOutsideBounds + distWithinBounds)) || (lastMarkerInBounds ? 100 : 0)
    );
  }


  getPath(): number[][] | undefined {
    return window.points;
  }
}

function inCircle(latLng: L.LatLng, cLatLng: L.LatLng, r: number): boolean {
  return latLng.distanceTo(cLatLng) < r
}

