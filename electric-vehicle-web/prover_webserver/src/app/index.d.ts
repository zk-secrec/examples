import { MapService } from "./map.service";

export {};

declare global {
  interface Window {
    MapService: MapService;
  }
}