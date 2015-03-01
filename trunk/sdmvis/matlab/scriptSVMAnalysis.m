% SDMVis script

% Options (TBD sometime ;-)
hint  ='Murinae vs. Gerbillinae';
%outdir='./';
%names_filename='temp_names.txt'

% Load names
[names_long names_short] = loadNames(names_filename,'_');
names = names_short;

global V;
global w;
global b;
global labels;
global names;

% Some PCA scatterplots
%figureMultiplot( 5, V,w,b, labels,names )

% Scatter plot matrix
group = grp2idx(labels);
colors = [104 155 255; 255 102 0]./255;
markers = [];
xnam = {'PC1','PC2','PC3','PC4','PC5','PC6','PC7','PC8'};
N = 6;
h = figure('Name',hint,'NumberTitle','off','ToolBar','none',...
           'Position',[-1000,200,700,600]);
gplotmatrix(V(:,1:N),[],group,colors,markers,[],'off','none', xnam(1:N) )
title(['Scatterplot matrix ' hint])

% Best separating axes
figurePC(1,3)