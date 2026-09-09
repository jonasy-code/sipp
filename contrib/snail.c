Object *sipp_snail(double cycle, double stretch, double r)
{
   Object *p = object_create();
   int u,v, un = 16, vn = 16;
   double uphi = (2*M_PI)/un, vphi = (2*M_PI)/vn;
   r *= .1;
   for(u=0; u<un; u++)
   {
      for(v=0; v<vn*cycle; v++)
      {
         double x,y,z;
         x = Pow(stretch,v/(2*M_PI))*(r*cos(v*vphi)+r*1.2*cos(v*vphi)*sin(u*uphi));
         y = Pow(stretch,v/(2*M_PI))*(r*sin(v*vphi)+r*1.2*sin(v*vphi)*sin(u*uphi));
         z = Pow(stretch,v/(2*M_PI))*(.6+.25*cos(u*uphi));
         vertex_push(x,y,z);
         u++;
         x = Pow(stretch,v/(2*M_PI))*(r*cos(v*vphi)+r*1.2*cos(v*vphi)*sin(u*uphi));
         y = Pow(stretch,v/(2*M_PI))*(r*sin(v*vphi)+r*1.2*sin(v*vphi)*sin(u*uphi));
         z = Pow(stretch,v/(2*M_PI))*(.6+.25*cos(u*uphi));
         vertex_push(x,y,z);
         v++;
         x = Pow(stretch,v/(2*M_PI))*(r*cos(v*vphi)+r*1.2*cos(v*vphi)*sin(u*uphi));
         y = Pow(stretch,v/(2*M_PI))*(r*sin(v*vphi)+r*1.2*sin(v*vphi)*sin(u*uphi));
         z = Pow(stretch,v/(2*M_PI))*(.6+.25*cos(u*uphi));
         vertex_push(x,y,z);
         u--;
         x = Pow(stretch,v/(2*M_PI))*(r*cos(v*vphi)+r*1.2*cos(v*vphi)*sin(u*uphi));
         y = Pow(stretch,v/(2*M_PI))*(r*sin(v*vphi)+r*1.2*sin(v*vphi)*sin(u*uphi));
         z = Pow(stretch,v/(2*M_PI))*(.6+.25*cos(u*uphi));
         vertex_push(x,y,z);
         v--;
         polygon_push();
      }
   }
   object_add_surface(p,surface_create(&default_surf_desc,default_shader));
   return(p);
}

static double Sgn(double x)
{
   if(x<0.) return(-1.); 
   return(1.); 
}

static double Pow(double x, double y)
{
   return(pow(x<0?-x:x,y)); 
}
