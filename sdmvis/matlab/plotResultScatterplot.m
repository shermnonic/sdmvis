function h = plotResultScatterplot( i,j, V, w,b, labels, names )

	plotPCAClasses( V, i,j, labels );
	plotSVM( w,b, i,j );
    plotPCANames( V+(ones(size(V))*0.01),i,j, names, 'w.' );
    axis equal; % tight	

end
