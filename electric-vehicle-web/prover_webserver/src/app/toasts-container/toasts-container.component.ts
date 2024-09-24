import { Component } from '@angular/core';
import {ToastsService} from '../toasts.service';

@Component({
  selector: 'app-toasts',
  templateUrl: './toasts-container.component.html',
  styleUrls: ['./toasts-container.component.scss'],
  host: { class: 'toast-container position-fixed top-0 end-0 p-3', style: 'z-index: 1200' },
})
export class ToastsContainerComponent {

  constructor(public toastService: ToastsService) {}

}
