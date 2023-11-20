import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { MainComponent } from './components/main/main.component';
import { AboutComponent } from './components/about/about.component';
import { CarInfoComponent } from './car-info/car-info.component';

const routes: Routes = [
  { path: 'home', component: MainComponent },
  { path: '', redirectTo: '/home', pathMatch: 'full' },
  { path: 'dashboard', redirectTo: '/home', pathMatch: 'full' },
  { path: 'about', component: AboutComponent },
  {path: 'info', component: CarInfoComponent}
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule],
})
export class AppRoutingModule {}
