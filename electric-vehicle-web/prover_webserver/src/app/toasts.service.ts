import { Injectable } from '@angular/core';

@Injectable({
  providedIn: 'root'
})
export class ToastsService {
	toasts: any[] = [];

	show(text: string, options: any = {}) {
		this.toasts.push({ text, ...options });
	}

	remove(toast: any) {
		this.toasts = this.toasts.filter((t) => t !== toast);
	}

	clear() {
		this.toasts.splice(0, this.toasts.length);
	}
}
