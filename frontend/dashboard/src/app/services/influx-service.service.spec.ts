import { TestBed } from '@angular/core/testing';

import { InfluxServiceService } from './influx-service.service';

describe('InfluxServiceService', () => {
  let service: InfluxServiceService;

  beforeEach(() => {
    TestBed.configureTestingModule({});
    service = TestBed.inject(InfluxServiceService);
  });

  it('should be created', () => {
    expect(service).toBeTruthy();
  });
});
