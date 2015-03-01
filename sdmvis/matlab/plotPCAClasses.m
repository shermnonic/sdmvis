function [] = plotPCAClasses( V, j1,j2, labels )
%PCAPLOTCLASSES Nice PCA scatterplot colored according to binary labels
%   
hold on;

for i=1:size(V,1)
    if labels(i) <= 0
        marker = 's';
        col = [104 155 255]./255;
        plot( V(i,j1),V(i,j2), marker,'Color',col,'LineWidth',1.5,...
            'MarkerEdgeColor',col,'MarkerFaceColor','none','MarkerSize',5)
    else
        marker = 'o';        
        col = [255 102 0]./255;
        plot( V(i,j1),V(i,j2), marker,'Color',col,...
            'MarkerEdgeColor',col,'MarkerFaceColor',col,'MarkerSize',5)
    end    
end

axis equal;
xlabel(sprintf('PC %d',j1));
ylabel(sprintf('PC %d',j2));

end
