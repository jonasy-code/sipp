Object *sipp_superquadric(a1,a2,a3,n,e,alpha)
double a1,a2,a3,n,e,alpha;
{
   int ue = 16, ve = 16;
   int u,v; double uphi = 2*M_PI/ue, vphi = 2*M_PI/ve;
   Object *p = object_create();
   
   if(alpha<=0.) vphi = M_PI/ve;
   if(n<=.1) n = .1;
   if(e<=.1) e = .1;
   for(v=-ve/2; v<=ve/2; v++)
   {
      for(u=-ue/2; u<=ue/2; u++)
      {
         double cu,su,cv,sv,x,y,z;
         cu = cos(u*uphi), su = sin(u*uphi), cv = cos(v*vphi), sv = sin(v*vphi);
         x = a1*(alpha+Pow(cv,n))*Pow(cu,e)*Sgn(cu)*Sgn(cv), y = a2*(alpha+Pow(cv,n))*Pow(su,e)*Sgn(su)*Sgn(cv), z = a3*Pow(sv,n)*Sgn(sv);
         u++;
         vertex_push(x,y,z);
         cu = cos(u*uphi), su = sin(u*uphi), cv = cos(v*vphi), sv = sin(v*vphi);
         x = a1*(alpha+Pow(cv,n))*Pow(cu,e)*Sgn(cu)*Sgn(cv), y = a2*(alpha+Pow(cv,n))*Pow(su,e)*Sgn(su)*Sgn(cv), z = a3*Pow(sv,n)*Sgn(sv);
         vertex_push(x,y,z);
         v++;
         cu = cos(u*uphi), su = sin(u*uphi), cv = cos(v*vphi), sv = sin(v*vphi);
         x = a1*(alpha+Pow(cv,n))*Pow(cu,e)*Sgn(cu)*Sgn(cv), y = a2*(alpha+Pow(cv,n))*Pow(su,e)*Sgn(su)*Sgn(cv), z = a3*Pow(sv,n)*Sgn(sv);
         vertex_push(x,y,z);
         u--;
         cu = cos(u*uphi), su = sin(u*uphi), cv = cos(v*vphi), sv = sin(v*vphi);
         x = a1*(alpha+Pow(cv,n))*Pow(cu,e)*Sgn(cu)*Sgn(cv), y = a2*(alpha+Pow(cv,n))*Pow(su,e)*Sgn(su)*Sgn(cv), z = a3*Pow(sv,n)*Sgn(sv);
         vertex_push(x,y,z);
         v--;
         polygon_push();
      }
   }
   object_add_surface(p,surface_create(&default_surf_desc,default_shader));
   return(p);
}
