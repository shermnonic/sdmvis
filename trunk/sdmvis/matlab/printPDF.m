function printPDF( h, filename )

set(h,'PaperUnits','centimeters')
set(h,'PaperSize',[29.7,21.0])

textobj = findobj('type','text');
set(textobj,'fontunits','points');
set(textobj,'fontsize',10);
set(textobj,'color', [.42 .42 .42] );

set(gcf,'PaperPositionMode','auto')
print('-dpdf','-r1200', filename);

end
