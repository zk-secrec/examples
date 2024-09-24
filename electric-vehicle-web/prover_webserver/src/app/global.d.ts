export {};
import { MapService } from "./map.service";


declare global{
    interface Window {
        mapService: MapService;
        points : any;
        signature : any;
        message : any;
        refreshPoints : () => void;
    }
}

  