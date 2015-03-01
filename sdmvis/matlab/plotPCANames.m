function [ p ] = plotPCANames( V, i, j, names, linespec )
%PLOTPCANAMES PCA scatterplot with text labels
%
if( nargin < 5 ) linespec = '.'; end;
p=plot(V(:,i),V(:,j),linespec); 
text(V(:,i),V(:,j),names);
xlabel(sprintf('PC %d',i)); ylabel(sprintf('PC %d',j));
%title(sprintf('%s - PC %d vs %d',hint,i,j));

end
