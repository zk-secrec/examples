import { Component, AfterViewInit } from '@angular/core';
import 'leaflet-editable';
import {MapService} from '../map.service';


@Component({
  selector: 'app-map',
  templateUrl: './map.component.html',
  styleUrls: ['./map.component.scss']
})
export class MapComponent implements AfterViewInit {

  constructor(private mapService: MapService) {}

  ngAfterViewInit(): void {
    this.mapService.initializeLeafletMap();
  }

}
