import { Component, EventEmitter, Output } from '@angular/core';

@Component({
  selector: 'app-main',
  templateUrl: './main.component.html',
  styleUrls: ['./main.component.css'],
})
export class MainComponent {
  @Output() refreshChange = new EventEmitter<number>();
  @Output() lastChange = new EventEmitter<string>();

  selectedRefreshTime = '10s';
  selectedLastTime = '1h';
  refreshTimeNumber = 10;

  convertRefreshTime(_refreshTime: string): number {
    const timeMap: { [key: string]: number } = {
      '10s': 10,
      '30s': 30,
      '1m': 60,
      '5m': 5 * 60,
    };
    //console.log(timeMap[_refreshTime]);
    return timeMap[_refreshTime];
  }

  onRefreshChange(_refreshTime: string) {
    this.selectedRefreshTime = _refreshTime;
    this.refreshTimeNumber = this.convertRefreshTime(_refreshTime);
    //console.log(this.refreshTimeNumber);
    this.refreshChange.emit(this.convertRefreshTime(this.selectedRefreshTime));
  }

  onLastChange(_lastTime: string) {
    this.selectedLastTime = _lastTime;
    this.lastChange.emit(_lastTime);
  }
}
