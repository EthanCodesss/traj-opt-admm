#ifndef GRADIENT_ADMM_H
#define GRADIENT_ADMM_H

#include "Utils/CCDUtils.h"

//#include "HighOrderCCD/BVH/BVH.h"
//#include "HighOrderCCD/CCD/CCD.h"
//#include "HighOrderCCD/Distance.h"
//#include "HighOrderCCD/Distance_der.h"

#include <vector>

//#include <Eigen/SparseCholesky>
//#include <unsupported/Eigen/AutoDiff>
#include <unsupported/Eigen/KroneckerProduct>

PRJ_BEGIN

class Gradient_admm
{
  public:
    typedef Eigen::MatrixXd Data;
    typedef std::vector< std::tuple< int, std::pair<double,double>, Eigen::MatrixXd > > Tree;
    typedef std::tuple< int, std::pair<double,double>, Eigen::MatrixXd >  Node;
    /*
    typedef Eigen::Matrix<double,Eigen::Dynamic,1> inner_derivative_t;//3*(order_num+1)
    typedef Eigen::AutoDiffScalar<inner_derivative_t> inner_scalar_t;
    typedef Eigen::Matrix<inner_scalar_t,Eigen::Dynamic,1> derivative_t;
    typedef Eigen::AutoDiffScalar<derivative_t> scalar_t;
    typedef Eigen::Matrix<scalar_t,Eigen::Dynamic,1> Vec12;
    typedef Eigen::Matrix<scalar_t,1,3> Vec3;
    */

    static void spline_gradient(const Data& spline, const double& piece_time,
                                const Data& p_slack, const Eigen::VectorXd& t_slack, 
                                const Data& p_lambda, const Eigen::VectorXd& t_lambda,
                                const std::vector<std::vector<Eigen::Vector3d>>& c_lists,
                                const std::vector<std::vector<double>>& d_lists,
                                Eigen::VectorXd& grad, Eigen::MatrixXd& hessian)//all piece no fix
    { 
        //double energy=lambda*plane_barrier_energy(spline,c_lists, d_lists)+ lambda*bound_energy(spline,piece_time);
        Eigen::VectorXd g0,g1,g2, partgrad;
        Eigen::MatrixXd h0,h1,h2;
        double g_t,h_t;
        /* h = h1+h2 pg
               pg'   h_t
        */
        /*
           g = g1+g2
               g_t
        */
        //clock_t time0 = clock();
        plane_barrier_gradient(spline, c_lists, d_lists,g1, h1);
        //clock_t time1 = clock();
        //auto_bound_gradient(spline,piece_time, g2,h2);
        //clock_t time2 = clock();
        //time_gradient(spline, piece_time, g_t, h_t, partgrad);
        //clock_t time3 = clock();  

        bound_gradient(spline,piece_time, g2,h2, g_t, h_t, partgrad);
        //clock_t time2 = clock();


        //std::cout<<"gradtime10:"<<(time1-time0)/(CLOCKS_PER_SEC/1000)<<std::endl;
        //std::cout<<"gradtime21:"<<(time2-time1)/(CLOCKS_PER_SEC/1000)<<std::endl;
        //std::cout<<"gradtime32:"<<(time3-time2)/(CLOCKS_PER_SEC/1000)<<std::endl<<std::endl;

        g0=lambda*g1+lambda*g2;
        h0=lambda*h1+lambda*h2;

        g_t*=lambda;
        h_t*=lambda;

        Eigen::Matrix3d I;
        I.setIdentity();

        Eigen::MatrixXd B,M; 

        for(int sp_id=0;sp_id<piece_num;sp_id++)
        {
            int init=sp_id*(order_num-2);

            Data p_delta = convert_list[sp_id]*spline.block<order_num+1,3>(init,0)
                            -p_slack.block<order_num+1,3>(sp_id*(order_num+1),0);

            Data lamb_part=p_lambda.block<order_num+1,3>(sp_id*(order_num+1),0);
            /*
            energy+=mu/2.0*p_delta.squaredNorm();
            energy+=mu/2.0*std::pow((piece_time-t_slack(sp_id)),2);

            for(int j=0;j<3;j++)
            {
                Eigen::VectorXd x=p_delta.col(j);
                Eigen::VectorXd lamb=lamb_part.col(j);
                energy+=lamb.transpose()*x;
            }
            energy+=t_lambda(sp_id)*(piece_time-t_slack(sp_id));
            */
            
            Eigen::MatrixXd x1=convert_list[sp_id].transpose()*p_delta;
            Eigen::MatrixXd x2=convert_list[sp_id].transpose()*lamb_part;

            M=convert_list[sp_id].transpose()*convert_list[sp_id];
            B = Eigen::kroneckerProduct(M,I);
            
            x1.transposeInPlace();
            x2.transposeInPlace();
            
            Eigen::VectorXd v1(Eigen::Map<Eigen::VectorXd>(x1.data(), 3*(order_num+1)));
            Eigen::VectorXd v2(Eigen::Map<Eigen::VectorXd>(x2.data(), 3*(order_num+1)));
            
            g0.segment<3*(order_num+1)>(3*init)+=mu*v1+v2;
            
            h0.block<3*(order_num+1),3*(order_num+1)>(3*init,3*init)+=mu*B;

            g_t+=mu*(piece_time-t_slack(sp_id)) + t_lambda(sp_id);
            h_t+=mu;
        }

        int n=3*trajectory_num;
        
        grad.resize(n+1);
        grad.head(n)=g0;
        grad(n)=g_t;

        hessian.resize(n+1,n+1);
        hessian.block(0,0,n,n)=h0;

        hessian.block(0,n,n,1)=lambda*partgrad;
        hessian.block(n,0,1,n)=lambda*partgrad.transpose();
        hessian(n,n)=h_t;
        /*
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(h0);
        Eigen::MatrixXd eigenvalue=eigensolver.eigenvalues();
        if(eigenvalue(0)<0)
        {
           std::cout<<"eigenslack:"<<eigenvalue(0)<<"-----------------------------"<<std::endl;
        }
        std::cout<<"h0 det:"<<h0.determinant()<<"\n";
        std::cout<<"h1 det:"<<h1.determinant()<<"\n";
        std::cout<<"h2 det:"<<h2.determinant()<<"\n";
        std::cout<<"spline det:"<<hessian.determinant()<<"\n\n";
        */
    }

    static void slack_gradient(const Data& c_spline, const double& piece_time,
                               const Data& p_part, const double& t_part, 
                               const Data& p_lambda, const double& t_lambda,
                               Eigen::VectorXd& grad, Eigen::MatrixXd& hessian)// one piece no fix
    { 
        Eigen::VectorXd g0,g1,g2, partgrad;
        Eigen::MatrixXd h0,h1,h2;
        double g_t,h_t;

        dynamic_gradient(p_part, t_part, 
                         g1, h1,
                         g_t, h_t, 
                         partgrad);
        /*
        energy+=mu/2.0*(c_spline-p_part).squaredNorm();
        energy+=mu/2.0*(piece_time-t_part)*(piece_time-t_part);

        for(int j=0;j<3;j++)
        {
            Eigen::VectorXd x=(c_spline-p_part).col(j);
            Eigen::VectorXd lamb=p_lambda.col(j);
            energy+=lamb.transpose()*x;
        }
        energy+=t_lambda*(piece_time-t_part);
        */
        int n=3*(order_num+1);

        Eigen::MatrixXd x1=p_part-c_spline;
        Eigen::MatrixXd x2=p_lambda;
        
        x1.transposeInPlace();
        x2.transposeInPlace();
            
        Eigen::VectorXd v1(Eigen::Map<Eigen::VectorXd>(x1.data(), n));
        Eigen::VectorXd v2(Eigen::Map<Eigen::VectorXd>(x2.data(), n));
        
        g2=mu*v1-v2;

        h2.resize(n,n); 
        h2.setIdentity();
        h2*=mu;

        g0=g1+g2;
        h0=h1+h2;

        g_t+=mu*(t_part-piece_time)-t_lambda;
        h_t+=mu;

        grad.resize(n+1);
        grad.head(n)=g0;
        grad(n)=g_t;

        hessian.resize(n+1,n+1);
        hessian.block(0,0,n,n)=h0;

        hessian.block(0,n,n,1)=partgrad;
        hessian.block(n,0,1,n)=partgrad.transpose();
        hessian(n,n)=h_t;

    }

    static void target_gradient(const Eigen::Vector3d& endpoint, const Eigen::Vector3d& target,
                                Eigen::VectorXd& grad, Eigen::MatrixXd& hessian)
    {
        Eigen::Matrix3d I;
        I.setIdentity();
        grad=endpoint-target;
        hessian=I;
    }

    static void dynamic_gradient(const Data& p_part, const double& t_part, 
                                 Eigen::VectorXd& grad, Eigen::MatrixXd& hessian,
                                 double& g_t, double& h_t, 
                                 Eigen::VectorXd& partgrad) //one piece
    {
        double dynamic_energy=0;

        //dynamic energy
        Eigen::Matrix3d I;
        I.setIdentity();

        Eigen::MatrixXd B = Eigen::kroneckerProduct(M_dynamic,I);
        Eigen::MatrixXd x=M_dynamic*p_part;

        x.transposeInPlace();
        Eigen::VectorXd v1(Eigen::Map<Eigen::VectorXd>(x.data(), 3*(order_num+1)));

        grad = ks/std::pow(t_part,2*der_num-1)*v1;
        hessian = ks/std::pow(t_part,2*der_num-1)*B;
        
        for(int j=0;j<3;j++)
        {
            Eigen::VectorXd x1=p_part.col(j);
            dynamic_energy+=ks/std::pow(t_part,2*der_num-1)*0.5*x1.transpose()*M_dynamic*x1;
        }

        //dynamic_energy+=kt*t_part;

        
        
        g_t=-(2*der_num-1)*dynamic_energy/t_part;
        g_t+=kt;
        //g_t+=2*kt*t_part;

        h_t=(2*der_num-1)*(2*der_num)*dynamic_energy/(t_part*t_part);  
        //h_t+=2*kt;

        partgrad=-(2*der_num-1)*grad/t_part;

        /*
        Eigen::MatrixXd h_;
        int n=3*(order_num+1);
        h_.resize(n+1,n+1);
        h_.block(0,0,n,n)=hessian;

        h_.block(0,n,n,1)=partgrad;
        h_.block(n,0,1,n)=partgrad.transpose();
        h_(n,n)=h_t;
        std::cout<<"slack det:"<<h_.determinant()<<"\n";
        */
    }

    static void plane_barrier_gradient(const Data& spline, 
                                       const std::vector<std::vector<Eigen::Vector3d>>& c_lists,
                                       const std::vector<std::vector<double>>& d_lists,
                                       Eigen::VectorXd& grad, Eigen::MatrixXd& hessian)
    {
        
        int num=3*trajectory_num;
        
        grad.resize(num);
        grad.setZero();

        hessian.resize(num,num);
        hessian.setZero();

        //Eigen::VectorXd auto_grad=grad;
        //Eigen::MatrixXd auto_hessian=hessian;
        double dmin=margin;
        for(unsigned int tr_id=0;tr_id<subdivide_tree.size();tr_id++)
        {        
            std::vector<Eigen::Vector3d> c_list=c_lists[tr_id];
            std::vector<double> d_list=d_lists[tr_id];
            if(c_list.size()==0)
                continue;

            int sp_id=std::get<0>(subdivide_tree[tr_id]);
            double weight=std::get<1>(subdivide_tree[tr_id]).second-std::get<1>(subdivide_tree[tr_id]).first;
            Eigen::MatrixXd basis=std::get<2>(subdivide_tree[tr_id]);
            int init=sp_id*(order_num-2);

            Eigen::MatrixXd bz;
            bz=spline.block<order_num+1,3>(init,0);
            
            Eigen::MatrixXd P=basis*bz;
            double d;

            for(unsigned int k=0;k<c_list.size();k++)
            {
                for(int j=0;j<=order_num;j++)
                {
                    //d=P[j].dot(c_list[k])+d_list[k];
                    d=P.row(j).dot(c_list[k])+d_list[k];
                    
                    if(d<margin)
                    { 
                       if(d<dmin)
                         dmin=d;
                       //energy+=weight*(1-d/margin*d/margin)*(1-d/margin*d/margin);  

                       Eigen::Matrix3d I; I.setIdentity();
                       Eigen::MatrixXd A=Eigen::kroneckerProduct(basis.row(j),I);
                        //std::cout<<A<<"\n";
                       Eigen::MatrixXd d_x=c_list[k].transpose()*A;

                       double e1=-weight*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d);

                       grad.segment(3*init,3*(order_num+1)) += e1*d_x.transpose();

                       double e2=-weight*(2*log(d/margin)+4*(d-margin)/d-(d-margin)*(d-margin)/(d*d));
                                        
                       hessian.block<3*(order_num+1),3*(order_num+1)>(3*init,3*init)+=e2*d_x.transpose()*d_x;

                       Eigen::MatrixXd h1=e2*d_x.transpose()*d_x;
                       //if(h1.determinant()<0)
                        // std::cout<<"plane det:"<<h1.determinant()<<"\n";
                    }
                }

            }
            
           
        }
        std::cout<<std::endl<<"dmin:"<<dmin<<std::endl;
    }
    
    static void bound_gradient(const Data& spline, const double& piece_time,
                               Eigen::VectorXd& grad, Eigen::MatrixXd& hessian,
                               double& g_t, double& h_t, 
                               Eigen::VectorXd& partgrad)
    {
        int num=3*trajectory_num;
        
        grad.resize(num);
        grad.setZero();

        hessian.resize(num,num);
        hessian.setZero();

        g_t=0;
        h_t=0;

        partgrad.resize(num);
        partgrad.setZero();
    
        double max_vel=0;
        double max_acc=0;

        for(unsigned int tr_id=0;tr_id<vel_tree.size();tr_id++)
        {
        
            int sp_id=std::get<0>(vel_tree[tr_id]);
            double weight=std::get<1>(vel_tree[tr_id]).second-std::get<1>(vel_tree[tr_id]).first;
            Eigen::MatrixXd basis=std::get<2>(vel_tree[tr_id]);
            
            int init=sp_id*(order_num-2);
            
            Eigen::MatrixXd bz;
            bz=spline.block<order_num+1,3>(init,0);
            
            Eigen::MatrixXd P=basis*bz;

            double d;
           
            for(int j=0;j<order_num;j++)
            {
                //Eigen::RowVector3d P_=P[j+1]-P[j];
                Eigen::RowVector3d P_=P.row(j+1)-P.row(j);
                Eigen::RowVector3d vel=order_num*P_;
                double v=vel.squaredNorm()/(weight*weight);
                //d=vel_limit*piece_time-vel.norm()/weight;
                d=vel_limit*vel_limit-v/(piece_time*piece_time);

                if(v/(piece_time*piece_time)>max_vel)
                  max_vel=v/(piece_time*piece_time);
                
                double vel_margin=2*vel_limit*margin;
                
                if(d<vel_margin)
                { 
                   double e1=-weight*(2*(d-vel_margin)*log(d/vel_margin)+(d-vel_margin)*(d-vel_margin)/d);
                   
                   double e2=-weight*(2*log(d/vel_margin)+4*(d-vel_margin)/d-(d-vel_margin)*(d-vel_margin)/(d*d));

                   //g_t, h_t
                   g_t+=2*e1*v/std::pow(piece_time,3);

                   h_t+=-6*e1*v/std::pow(piece_time,4)+
                        +4*e2*v*v/std::pow(piece_time,6);
                   
                   //grad, hessian
                   //-weight*(d-margin)*(d-margin)*log(d/margin)
                    Eigen::Matrix3d I; I.setIdentity();
                    Eigen::MatrixXd A=Eigen::kroneckerProduct(basis.row(j+1),I)-
                                      Eigen::kroneckerProduct(basis.row(j),I);
                    //std::cout<<A<<"\n";
                    Eigen::RowVector3d d_p;
                    Eigen::Matrix3d h_p;

                    
                    d_p=-2*order_num*order_num/std::pow(weight*piece_time,2)*P_;
        
                    h_p=-2*order_num*order_num/std::pow(weight*piece_time,2)*I;

                    Eigen::MatrixXd d_x=d_p*A;

                    
                    grad.segment(3*init,3*(order_num+1)) += e1*d_x.transpose();
                    
                    
                    hessian.block<3*(order_num+1),3*(order_num+1)>(3*init,3*init)+=e2*d_x.transpose()*d_x+e1*A.transpose()*h_p*A;

                    Eigen::MatrixXd h1=e2*d_x.transpose()*d_x+e1*A.transpose()*h_p*A;
                       

                    //partgrad
  
                    //2*e1*v/std::pow(piece_time,3);
                    //2*e1*(vel_limit*vel_limit-d)/piece_time;
                    double e3=-2*e1/piece_time
                              +2*e2*(vel_limit*vel_limit-d)/piece_time;

                    partgrad.segment(3*init,3*(order_num+1)) += e3*d_x.transpose();

                }
            }
        }

        
        for(unsigned int tr_id=0;tr_id<acc_tree.size();tr_id++)
        {
        
            int sp_id=std::get<0>(acc_tree[tr_id]);
            double weight=std::get<1>(acc_tree[tr_id]).second-std::get<1>(acc_tree[tr_id]).first;
            Eigen::MatrixXd basis=std::get<2>(acc_tree[tr_id]);
            
            int init=sp_id*(order_num-2);

            Eigen::MatrixXd bz;
            bz=spline.block<order_num+1,3>(init,0);
            
            Eigen::MatrixXd P=basis*bz;
            double d;
           
            for(int j=0;j<order_num-1;j++)
            {
                //Eigen::RowVector3d P_=P[j+2]-2*P[j+1]+P[j];
                Eigen::RowVector3d P_=P.row(j+2)-2*P.row(j+1)+P.row(j);
                Eigen::RowVector3d acc=order_num*(order_num-1)*P_;
                double a=acc.squaredNorm()/(weight*weight*weight*weight);
                
                //d=acc_limit*piece_time*piece_time-acc.norm()/(weight*weight);
                d=acc_limit*acc_limit-a/std::pow(piece_time*piece_time,2);

                if(a/std::pow(piece_time*piece_time,2)>max_acc)
                  max_acc=a/std::pow(piece_time*piece_time,2);
                
                double acc_margin=2*acc_limit*margin;
                
                if(d<acc_margin)
                { 
                   double e1=-weight*(2*(d-acc_margin)*log(d/acc_margin)+(d-acc_margin)*(d-acc_margin)/d);
                   
                   double e2=-weight*(2*log(d/acc_margin)+4*(d-acc_margin)/d-(d-acc_margin)*(d-acc_margin)/(d*d));

                   //g_t, h_t

                   g_t+=4*e1*a/std::pow(piece_time,5);

                   h_t+=-20*e1*a/std::pow(piece_time,6)+
                        +16*e2*a*a/std::pow(piece_time,10);

                   //grad, hessian
                   //-weight*(d-margin)*(d-margin)*log(d/margin)
                    Eigen::Matrix3d I; I.setIdentity();
                    Eigen::MatrixXd A=Eigen::kroneckerProduct(basis.row(j+2),I)-
                                      2*Eigen::kroneckerProduct(basis.row(j+1),I)+
                                      Eigen::kroneckerProduct(basis.row(j),I);
                    //std::cout<<A<<"\n";
                    Eigen::RowVector3d d_p;
                    Eigen::Matrix3d h_p;
                    
                    d_p=-2*std::pow(order_num*(order_num-1),2)/std::pow(weight*piece_time,4)*P_;

                    h_p=-2*std::pow(order_num*(order_num-1),2)/std::pow(weight*piece_time,4)*I;

                    Eigen::MatrixXd d_x=d_p*A;
                    
                    grad.segment(3*init,3*(order_num+1)) += e1*d_x.transpose();
                     
                    hessian.block<3*(order_num+1),3*(order_num+1)>(3*init,3*init)+=e2*d_x.transpose()*d_x+e1*A.transpose()*h_p*A;

                    Eigen::MatrixXd h1=e2*d_x.transpose()*d_x+e1*A.transpose()*h_p*A;
                    

                    //partgrad

                    //4*e1*a/std::pow(piece_time,5);
                    //4*e1*(acc_limit*acc_limit-d)/piece_time;
                    double e3=-4*e1/piece_time
                              +4*e2*(acc_limit*acc_limit-d)/piece_time;

                    partgrad.segment(3*init,3*(order_num+1)) += e3*d_x.transpose();

                }
            }
        }

        std::cout<<"max_vel:"<<sqrt(max_vel)<<std::endl;
        std::cout<<"max_acc:"<<sqrt(max_acc)<<std::endl;
    }
    
    /*
    static void bound_gradient(const Data& spline, const double& piece_time,
                               Eigen::VectorXd& grad, Eigen::MatrixXd& hessian,
                               double& g_t, double& h_t, 
                               Eigen::VectorXd& partgrad)
    {
        int num=3*trajectory_num;
        
        grad.resize(num);
        grad.setZero();

        hessian.resize(num,num);
        hessian.setZero();

        g_t=0;
        h_t=0;

        partgrad.resize(num);
        partgrad.setZero();
    
        double max_vel=0;
        double max_acc=0;

        for(unsigned int tr_id=0;tr_id<vel_tree.size();tr_id++)
        {
        
            int sp_id=std::get<0>(vel_tree[tr_id]);
            double weight=std::get<1>(vel_tree[tr_id]).second-std::get<1>(vel_tree[tr_id]).first;
            Eigen::MatrixXd basis=std::get<2>(vel_tree[tr_id]);
            
            int init=sp_id*(order_num-2);
            
            Eigen::MatrixXd bz;
            bz=spline.block<order_num+1,3>(init,0);
            
            Eigen::MatrixXd P=basis*bz;

            double d;
           
            for(int j=0;j<order_num;j++)
            {
                //Eigen::RowVector3d P_=P[j+1]-P[j];
                Eigen::RowVector3d P_=P.row(j+1)-P.row(j);
                Eigen::RowVector3d vel=order_num*P_;
                double v=vel.norm()/(weight);
                //d=vel_limit*piece_time-vel.norm()/weight;
                d=vel_limit-v/piece_time;

                if(v/piece_time>max_vel)
                  max_vel=v/piece_time;
                
                if(d<margin)
                { 
                   double e1=-weight*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d);
                   
                   double e2=-weight*(2*log(d/margin)+4*(d-margin)/d-(d-margin)*(d-margin)/(d*d));

                   //g_t, h_t
                   g_t+=e1*v/std::pow(piece_time,2);

                   h_t+=-2*e1*v/std::pow(piece_time,3)+
                        +e2*v*v/std::pow(piece_time,4);

                   //g_t, h_t
                   //g_t+=weight*v/(piece_time*piece_time)*(-2*(d-margin)*log(d/margin)-(d-margin)*(d-margin)/d);

                   //h_t+=weight*(-2*v/std::pow(piece_time,3)*(-2*(d-margin)*log(d/margin)-(d-margin)*(d-margin)/d)+
                    //        v*v/std::pow(piece_time,4)*(-2*log(d/margin)-4*(d-margin)/d+(d-margin)*(d-margin)/(d*d)));
                    
                   //grad, hessian
                   //-weight*(d-margin)*(d-margin)*log(d/margin)
                    Eigen::Matrix3d I; I.setIdentity();
                    Eigen::MatrixXd A=Eigen::kroneckerProduct(basis.row(j+1),I)-
                                      Eigen::kroneckerProduct(basis.row(j),I);
                    //std::cout<<A<<"\n";
                    Eigen::RowVector3d d_p;
                    Eigen::Matrix3d h_p;

                    double d_=P_.norm();
                    
                    d_p=-order_num/(weight*piece_time)*P_/d_;
        
                    h_p=-order_num/(weight*piece_time)*(I/d_-P_.transpose()*P_/std::pow(d_,3));

                    Eigen::MatrixXd d_x=d_p*A;

                    //double e1=-weight*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d);
                    
                    grad.segment(3*init,3*(order_num+1)) += e1*d_x.transpose();
                    
                    //double e2=-weight*(2*log(d/margin)+4*(d-margin)/d-(d-margin)*(d-margin)/(d*d));
                    
                    hessian.block<3*(order_num+1),3*(order_num+1)>(3*init,3*init)+=e2*d_x.transpose()*d_x+e1*A.transpose()*h_p*A;

                    Eigen::MatrixXd h1=e2*d_x.transpose()*d_x+e1*A.transpose()*h_p*A;
                       //if(h1.determinant()<0)
                         //std::cout<<"vel det:"<<h1.determinant()<<"\n";

                    //partgrad
  
                    //weight/piece_time*(d-vel_limit)*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d);
                    //e1*v/std::pow(piece_time,2);
                    //e1*(vel_limit-d)/piece_time;
                    double e3=-e1/piece_time
                              +e2*(vel_limit-d)/piece_time;
                    //double e3=weight/piece_time*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d)
                    //         +weight/piece_time*(d-vel_limit)*(2*log(d/margin)+4*(d-margin)/d-(d-margin)*(d-margin)/(d*d));

                    partgrad.segment(3*init,3*(order_num+1)) += e3*d_x.transpose();

                }
            }
        }

        
        for(unsigned int tr_id=0;tr_id<acc_tree.size();tr_id++)
        {
        
            int sp_id=std::get<0>(acc_tree[tr_id]);
            double weight=std::get<1>(acc_tree[tr_id]).second-std::get<1>(acc_tree[tr_id]).first;
            Eigen::MatrixXd basis=std::get<2>(acc_tree[tr_id]);
            
            int init=sp_id*(order_num-2);

            Eigen::MatrixXd bz;
            bz=spline.block<order_num+1,3>(init,0);
            
            Eigen::MatrixXd P=basis*bz;
            double d;
           
            for(int j=0;j<order_num-1;j++)
            {
                //Eigen::RowVector3d P_=P[j+2]-2*P[j+1]+P[j];
                Eigen::RowVector3d P_=P.row(j+2)-2*P.row(j+1)+P.row(j);
                Eigen::RowVector3d acc=order_num*(order_num-1)*P_;
                double a=acc.norm()/(weight*weight);
                
                //d=acc_limit*piece_time*piece_time-acc.norm()/(weight*weight);
                d=acc_limit-a/(piece_time*piece_time);

                if(a/(piece_time*piece_time)>max_acc)
                  max_acc=a/(piece_time*piece_time);
                
                if(d<margin)
                { 
                   double e1=-weight*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d);
                   
                   double e2=-weight*(2*log(d/margin)+4*(d-margin)/d-(d-margin)*(d-margin)/(d*d));

                   //g_t, h_t
                   g_t+=2*e1*a/std::pow(piece_time,3);

                   h_t+=-6*e1*a/std::pow(piece_time,4)+
                        +4*e2*a*a/std::pow(piece_time,6);
                   //g_t, h_t
                   //g_t+=weight*2*a/std::pow(piece_time,3)*(-2*(d-margin)*log(d/margin)-(d-margin)*(d-margin)/d);

                   //h_t+=weight*(-6*a/std::pow(piece_time,4)*(-2*(d-margin)*log(d/margin)-(d-margin)*(d-margin)/d)
                    //            +4*a*a/std::pow(piece_time,6)*(-2*log(d/margin)-4*(d-margin)/d+(d-margin)*(d-margin)/(d*d)));

                   //grad, hessian
                   //-weight*(d-margin)*(d-margin)*log(d/margin)
                    Eigen::Matrix3d I; I.setIdentity();
                    Eigen::MatrixXd A=Eigen::kroneckerProduct(basis.row(j+2),I)-
                                      2*Eigen::kroneckerProduct(basis.row(j+1),I)+
                                      Eigen::kroneckerProduct(basis.row(j),I);
                    //std::cout<<A<<"\n";
                    Eigen::RowVector3d d_p;
                    Eigen::Matrix3d h_p;

                    double d_=P_.norm();
                    
                    d_p=-order_num*(order_num-1)/std::pow(weight*piece_time,2)*P_/d_;
        
                    h_p=-order_num*(order_num-1)/std::pow(weight*piece_time,2)*(I/d_-P_.transpose()*P_/std::pow(d_,3));

                    Eigen::MatrixXd d_x=d_p*A;

                    //double e1=-weight*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d);
                    
                    grad.segment(3*init,3*(order_num+1)) += e1*d_x.transpose();
                    
                    //double e2=-weight*(2*log(d/margin)+4*(d-margin)/d-(d-margin)*(d-margin)/(d*d));
                    
                    hessian.block<3*(order_num+1),3*(order_num+1)>(3*init,3*init)+=e2*d_x.transpose()*d_x+e1*A.transpose()*h_p*A;

                    Eigen::MatrixXd h1=e2*d_x.transpose()*d_x+e1*A.transpose()*h_p*A;
                       //if(h1.determinant()<0)
                         //std::cout<<"acc det:"<<h1.determinant()<<"\n";

                    //partgrad

                    //2*weight/piece_time*(d-acc_limit)*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d);
                    //2*e1*a/std::pow(piece_time,3);
                    //2*e1*(acc_limit-d)/piece_time;
                    double e3=-2*e1/piece_time
                              +2*e2*(acc_limit-d)/piece_time;

                    //double e3=2*weight/piece_time*(2*(d-margin)*log(d/margin)+(d-margin)*(d-margin)/d)
                    //         +2*weight/piece_time*(d-acc_limit)*(2*log(d/margin)+4*(d-margin)/d-(d-margin)*(d-margin)/(d*d));

                    partgrad.segment(3*init,3*(order_num+1)) += e3*d_x.transpose();

                }
            }
        }

        std::cout<<"max_vel:"<<max_vel<<std::endl;
        std::cout<<"max_acc:"<<max_acc<<std::endl;
    }
    */
};

PRJ_END

#endif