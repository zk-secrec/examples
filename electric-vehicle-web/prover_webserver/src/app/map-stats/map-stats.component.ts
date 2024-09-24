import { Component } from '@angular/core';
import {MapService} from '../map.service';

@Component({
  selector: 'app-map-stats',
  templateUrl: './map-stats.component.html',
  styleUrls: ['./map-stats.component.scss']
})
export class MapStatsComponent {

  distance$ = this.mapService.pathDistance$;
  inBounds$ = this.mapService.pathInBounds$;

  constructor(private mapService: MapService) {
    this.inBounds$.subscribe((val) => {console.log(`bounds percentage: ${val}`);})
    this.distance$.subscribe((val) => {console.log(`total dist: ${val}`);})
  }

}
