function [h_line h_arrow] = plotSVMPlane( w, b, i,j )
%PLOTPLANE Plot SVM hyperplane in 2D (i.e. a line)
%   
    % options
    plane_color = [.3, .9, .3];
	% [.7, .7, .7];
	% [.3, .9, .3];
    arrow_color = plane_color;
    margin_color = plane_color;    
    ext = 0.6; % related to line length

    % normalize w *and* b
    w0 = w / norm(w);
    b0 = b / norm(w);
    
    % consider only dimensions i,j
    w0 = [w0(i); w0(j)];  
   
    % point on plane    
    p0 = -b0*w0; % FIXME: why minus b?
    
    % plot plane    
    w0_perp = [0 -1; 1 0]*w0;
    P = zeros(2,2);
    P(:,1) = p0 + ext*w0_perp;
    P(:,2) = p0 - ext*w0_perp;
    h_line = plot( P(1,:), P(2,:), 'Color',plane_color, 'LineWidth',1.5 );
    
    % plot margin  (margin is 1/|w|)
    margin = 1 / norm(w);
    Mu(:,1) = (p0 + margin*w0) + ext*w0_perp;
    Mu(:,2) = (p0 + margin*w0) - ext*w0_perp;
    plot( Mu(1,:), Mu(2,:), 'Color',margin_color, 'LineWidth',1.5, ...
          'LineStyle', '--');
    Md(:,1) = (p0 - margin*w0) + ext*w0_perp;
    Md(:,2) = (p0 - margin*w0) - ext*w0_perp;
    plot( Md(1,:), Md(2,:), 'Color',margin_color, 'LineWidth',1.5, ...
          'LineStyle', '--');
    
    % plot normal
    h_arrow = quiver( p0(1),p0(2), 0.2*w0(1), 0.2*w0(2), ...
            'Color',arrow_color, 'LineWidth',1.5, 'MaxHeadSize', 4 );


end
