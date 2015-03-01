function [] = plotSVM( w, b, i,j )
%PLOTSVM Nice plot of SVM separating hyperplane (e.g. into PCA scatter plot)
%   SVM hyperplane is given by normal w and distance to origin b.
%   See also plotPCAClasses().
hold on;

    plotSVMPlane( w,b,i,j );

%     % normalize w *and* b
%     w0 = w / norm(w);
%     b0 = b / norm(w);
%     
%     % consider only dimensions i,j
%     w0 = [w0(i); w0(j)];  
%    
%     % point on plane    
%     p0 = -b0*w0;                % FIXME: why minus b?
%     
%     wcolor = [.3, .9, .3];
% 
%     % plot plane    
%     w0_perp = [0 -1; 1 0]*w0;
%     P = zeros(2,2);
%     P(:,1) = p0 + 0.6*w0_perp;
%     P(:,2) = p0 - 0.6*w0_perp;
%     plot( P(1,:), P(2,:), 'Color',wcolor, 'LineWidth',1.5, 'LineStyle','--' );
%     
%     % plot normal
%     quiver( p0(1),p0(2), 0.2*w0(1), 0.2*w0(2), ...
%             'Color',wcolor, 'LineWidth',1.5, 'MaxHeadSize', 4 );
end
