import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';

import { AppComponent } from './app.component';
import { MapComponent } from './map/map.component';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { MapStatsComponent } from './map-stats/map-stats.component';
import { ProofComponent } from './proof/proof.component';
import {MapService} from './map.service';
import {ProofService} from './proof.service';
import { ToastsContainerComponent } from './toasts-container/toasts-container.component';
import {ToastsService} from './toasts.service';

@NgModule({
  declarations: [
    AppComponent,
    MapComponent,
    MapStatsComponent,
    ProofComponent,
    ToastsContainerComponent
  ],
  imports: [
    BrowserModule,
    NgbModule
  ],
  providers: [
    MapService,
    ProofService,
    ToastsService
  ],
  bootstrap: [AppComponent]
})
export class AppModule { }
