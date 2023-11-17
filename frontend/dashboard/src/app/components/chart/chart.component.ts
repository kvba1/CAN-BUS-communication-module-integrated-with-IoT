import {
  Component,
  OnInit,
  ChangeDetectorRef,
  Input,
  OnChanges,
  SimpleChanges,
  SimpleChange,
} from '@angular/core';
import { ChartConfiguration, ChartOptions, ChartDataset } from 'chart.js';
import { InfluxServiceService } from 'src/app/services/influx-service.service';
import 'chartjs-adapter-date-fns';
import { interval, Subscription } from 'rxjs';
import 'chartjs-plugin-zoom';

@Component({
  selector: 'app-chart',
  templateUrl: './chart.component.html',
  styleUrls: ['./chart.component.css'],
})
export class ChartComponent implements OnInit, OnChanges {
  @Input() dataType: string;
  @Input() refreshTime: number;
  @Input() lastTime: string;

  private colorMap: { [key: string]: string } = {
    Speed: '#FF0000', // Red for Speed
    RPM: '#0000FF', // Blue for RPM
    EngineTemperature: '#800080',
    // ... add more mappings for other data types
  };

  private dataRefreshSubscription: Subscription | undefined;
  refreshInterval: number = 10000;

  constructor(
    private influxService: InfluxServiceService,
    private changeDetectorRef: ChangeDetectorRef
  ) {
    this.dataType = 'Speed';
    this.refreshTime = 10;
    this.lastTime = '1h';
    this.refreshInterval = this.refreshTime * 1000;
  }

  ngOnChanges(changes: SimpleChanges): void {
    let dataTypeChanged =
      changes['dataType']?.currentValue !== changes['dataType']?.previousValue;
    let rangeChanged =
      changes['lastTime']?.currentValue !== changes['lastTime']?.previousValue;
    let refreshTimeChanged =
      changes['refreshTime']?.currentValue !==
      changes['refreshTime']?.previousValue;

    if (refreshTimeChanged) {
      console.log(this.refreshTime);
      this.refreshInterval = this.refreshTime * 1000;
      this.restartDataRefreshSubscription();
    }

    if (dataTypeChanged || rangeChanged) {
      console.log(this.lastTime);
      this.loadData(this.dataType, this.lastTime);
    }
  }

  updateChart(newLastTime: string) {
    // Logic to update the chart based on new lastTime
    console.log('Updating chart with last time:', newLastTime);
    // For example, fetch new data and redraw the chart
  }

  private classicColors: string[] = [
    '#FF0000', // Red
    '#0000FF', // Blue
    '#008000', // Green
    '#FFFF00', // Yellow
    '#FFA500', // Orange
    '#800080', // Purple
    // ... add more classic colors if needed
  ];

  public isChartDataLoaded = false; // Flag to control chart rendering

  public lineChartData: ChartConfiguration<'line'>['data'] | undefined;

  public lineChartLegend = true;

  public lineChartOptions: ChartOptions<'line'> = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      zoom: {
        pan: {
          enabled: true,
          mode: 'xy',
        },
        zoom: {
          //enabled: true,
          mode: 'xy',
        },
      },
      legend: {
        position: 'top',
        labels: {
          usePointStyle: true,
          padding: 20,
        },
      },
      tooltip: {
        mode: 'index',
        intersect: false,
        bodySpacing: 5,
        titleSpacing: 15,
        padding: 15,
      },
    },
    scales: {
      x: {
        type: 'time',
        grid: {
          display: false,
          drawBorder: false,
        },
        ticks: {
          maxTicksLimit: 20,
          color: '#666',
        },
      },
      y: {
        grid: {
          drawBorder: false,
          color: (context) =>
            context.tick.value === 0 ? '#000' : 'rgba(0, 0, 0, 0.1)',
        },
        ticks: {
          color: '#666',
        },
      },
    },
    elements: {
      line: {
        tension: 0.4, // Smoothens the line
      },
      point: {
        radius: 3, // Size of points
        hoverRadius: 5, // Size when hovered
        hitRadius: 5, // Clickable area radius
      },
    },
  };

  ngOnInit() {
    this.restartDataRefreshSubscription();
  }

  private restartDataRefreshSubscription() {
    // Unsubscribe from the existing subscription if it exists
    if (this.dataRefreshSubscription) {
      this.dataRefreshSubscription.unsubscribe();
    }

    // Create a new subscription
    this.dataRefreshSubscription = interval(this.refreshInterval).subscribe(
      () => {
        this.loadData(this.dataType, this.lastTime);
      }
    );
  }

  ngOnDestroy() {
    if (this.dataRefreshSubscription) {
      this.dataRefreshSubscription.unsubscribe();
    }
  }

  loadData(dataType: string, range: string) {
    this.influxService.queryData(dataType, range).subscribe({
      next: (response: string) => {
        const parsedData = this.parseCSVData(response);
        const chartColor =
          this.colorMap[dataType] || this.getRandomClassicColor();
        // Use the predefined color for the dataType, or a random color if not defined

        this.lineChartData = {
          labels: parsedData.labels,
          datasets: [
            {
              data: parsedData.values,
              label: dataType,
              fill: true,
              tension: 0.4,
              borderColor: chartColor,
              backgroundColor: chartColor + '33', // Adds transparency
              pointBorderColor: chartColor,
              pointBackgroundColor: '#fff',
              pointHoverBackgroundColor: chartColor,
              pointHoverBorderColor: '#fff',
            },
          ],
        };
        this.isChartDataLoaded = true; // Set flag to true after data is loaded
        this.changeDetectorRef.detectChanges(); // Trigger change detection
      },
      error: (error: any) => {
        console.error('There was an error!', error);
      },
    });
  }

  parseCSVData(csvData: string) {
    const lines = csvData.split('\n');
    const labels = [];
    const values = [];

    for (let i = 1; i < lines.length; i++) {
      const line = lines[i].split(',');
      if (line.length > 1) {
        const timestamp = new Date(line[5]); // Parse the timestamp
        labels.push(timestamp); // Use the formatted time
        values.push(parseFloat(line[6])); // Assuming the value is at index 5
      }
    }
    console.log(values);
    return { labels, values };
  }

  private getRandomClassicColor() {
    const randomIndex = Math.floor(Math.random() * this.classicColors.length);
    return this.classicColors[randomIndex];
  }
}
