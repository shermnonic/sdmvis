function [ names names_short ] = loadNames( fname, sep )
%LOADNAMES Load newline separated names from a textfile
%   
if nargin < 2, sep='_'; end

fid   = fopen(fname,'rt');
if fid==-1
    error(['Could not open ',fname]);
end
tline = fgetl(fid);
numNames=0;
while ischar(tline)
    numNames = numNames + 1;
    names(numNames) = {tline};
    
	if nargout > 1
		% old short name variant
		names_short(numNames) = {tline(1:5)};    
		
		% build short name from first characters of first tokens (newline sep)
		tokens = regexp( tline, sep, 'split' );
		n2 = min(3,length(tokens{2}));
		names_short(numNames) = { [ tokens{1}(1:3) '.' tokens{2}(1:n2) ] };
	end
    
    tline = fgetl(fid);
end
fclose(fid);

end
