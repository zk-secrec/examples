import {Component, OnInit} from '@angular/core';
import {Observable} from 'rxjs';
import {combineLatestWith, map} from 'rxjs/operators';
import {ProofService} from '../proof.service';


@Component({
  selector: 'app-proof',
  templateUrl: './proof.component.html',
  styleUrls: ['./proof.component.scss']
})
export class ProofComponent implements OnInit {

  progressValue$: Observable<number>;
  proofDone$: Observable<boolean>;
  proofStatus$: Observable<boolean>;
  proofStatusString$: Observable<string>;

  constructor(private proofService: ProofService) {
    this.progressValue$ = this.proofService.progressValue$;
    this.proofDone$ = this.proofService.proofDone$;
    this.proofStatus$ = this.proofService.proofStatus$;
    this.proofStatusString$ = this.statusToBootstrapType$();
  }

  ngOnInit(): void { }

  proveClicked(): void {
    this.proofService.executeProof();
  }

  loadFirstPath(): void {
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
    window.refreshPoints();
  }

  loadSecondPath(): void {
    var pentagon_capitol_paris = [
      [1295213,435262],
      [1295292,435714],
      [1295286,435731],
      [1295301,435742],
      [1295626,436320],
      [1296352,436456],
      [1296836,435881],
      [1296892,435770],
      [1296966,435310],
      [1297082,434658],
      [1297092,434659],
      [1297372,434561],
      [1297367,434558],
      [1297366,434558],
      [1297641,434598],
      [1297690,435811],
      [1298644,437311],
      [1299769,438765],
      [1300595,439781],
      [1301564,441084],
      [1302725,442251],
      [1303259,443625],
      [1303277,444576],
      [1303276,444574],
      [1303967,444624],
      [1304243,444626],
      [1304242,444624],
      [1304564,444625],
      [1305885,444624],
      [1306016,444623],
      [1306066,444622],
      [1306612,444637],
      [1307606,444630],
      [1308655,444624],
    ]
    window.points = pentagon_capitol_paris;
    window.refreshPoints();

  }

  statusToBootstrapType$(): Observable<string> {
    return this.proofDone$.pipe(
      combineLatestWith(this.proofStatus$),
      map(([done, status]) => done ? (status ? "success" : "danger") : "primary")
    );
  }
}
